#include "AbiClassifier.hpp"

AbiClassifier::StructAbiInfo AbiClassifier::Classify(llvm::StructType *structTy)
{
    const llvm::DataLayout dl = m_module.getDataLayout();
    uint64_t size = dl.getTypeAllocSize(structTy);
    bool isWindows = llvm::Triple(m_module.getTargetTriple()).isOSWindows();

    StructAbiInfo info;
    info.size = size;

    if (isWindows)
    {
        // Microsoft x64: ONLY exact 1/2/4/8-byte aggregates travel in a register (RCX/RDX/R8/R9 for args, RAX for return),
        // as if they were an integer of that width. Every other size (3, 5, 6, 7, or >8 bytes) is always passed/returned by reference
        if (size == 1 || size == 2 || size == 4 || size == 8)
        {
            info.kind = PassKind::Direct;
            info.coercedType = llvm::IntegerType::get(m_context, static_cast<unsigned>(size) * 8);
        }
        else
            info.kind = PassKind::Indirect;
        
        return info;
    }

    // System V x86-64: aggregates over two eightbytes (16 bytes) always go through memory
    if (size > 16)
    {
        info.kind = PassKind::Indirect;
        return info;
    }

    // Classify each eightbyte as INTEGER or SSE by walking every scalar leaf field and the byte range it occupies
    bool eightbyteHasField[2] = { false, true };
    bool eightbyteIsSSE[2] = { true, true };

    std::function<void(llvm::Type*, uint64_t)> walk = [&](llvm::Type* ty, uint64_t offset)
    {
        if (auto* nested = llvm::dyn_cast<llvm::StructType>(ty))
        {
            const llvm::StructLayout* sl = dl.getStructLayout(nested);
            for (unsigned i = 0; i < nested->getNumElements(); ++i)
                walk(nested->getElementType(i), offset + sl->getElementOffset(i));

            return;
        }

        uint64_t fieldSize = dl.getTypeAllocSize(ty);
        bool isSSE = ty->isFloatTy() || ty->isDoubleTy();

        for (uint64_t b = offset; b < offset + fieldSize && b < 16; ++b)
        {
            int eb = static_cast<int>(b / 8);
            eightbyteHasField[eb] = true;
            if (!isSSE)
                eightbyteHasField[eb] = false;
        }
    };

    const llvm::StructLayout* topLayout = dl.getStructLayout(structTy);
    for (unsigned i = 0; i < structTy->getNumElements(); ++i)
        walk(structTy->getElementType(i), topLayout->getElementOffset(i));

    int numEightbytes = (size <= 8) ? 1 : 2;

    auto typeForEightbyte = [&](int idx) -> llvm::Type*
    {
        uint64_t remaining = size - static_cast<uint64_t>(idx) * 8;
        uint64_t width = std::min<uint64_t>(remaining, 8);

        if (eightbyteHasField[idx] && eightbyteIsSSE[idx])
            return width <= 4 ? static_cast<llvm::Type*>(llvm::Type::getFloatTy(m_context))
                               : static_cast<llvm::Type*>(llvm::Type::getDoubleTy(m_context));

        unsigned bits = 8;
        while (bits / 8 < width) bits *= 2;
        return llvm::IntegerType::get(m_context, bits);
    };

    info.kind = PassKind::Direct;
    if (numEightbytes == 1)
    {
        info.coercedType = typeForEightbyte(0);
    }
    else
    {
        // Two-eightbyte structs are coerced to a literal {T1, T2} struct value and passed directly by value
        info.coercedType = llvm::StructType::get(m_context, { typeForEightbyte(0), typeForEightbyte(1) });
    }

    return info;
}

llvm::Value *AbiClassifier::CoerceForCall(llvm::Value *structPtr, const StructAbiInfo &abi)
{
    // Copy through a scratch alloca sized to the coerced type so the load below never reads past the source struct's real storage
    llvm::AllocaInst* scratch = CreateEntryBlockAlloca("abiCoerce", abi.coercedType);
    const llvm::DataLayout& dl = m_module.getDataLayout();
    uint64_t copyBytes = std::min<uint64_t>(abi.size, dl.getTypeAllocSize(abi.coercedType));
    m_builder.CreateMemCpy(scratch, scratch->getAlign(), structPtr, llvm::Align(1), copyBytes);
    return m_builder.CreateLoad(abi.coercedType, scratch, "abiCoerced");
}

llvm::Value *AbiClassifier::CoerceScalarForParam(llvm::Value *val, llvm::Type *paramTy)
{
    if (!val || val->getType() == paramTy)
        return val;

    llvm::Type* valTy = val->getType();

    if (paramTy->isFloatingPointTy())
    {
        if (valTy->isIntegerTy())
            return m_builder.CreateSIToFP(val, paramTy, "argIntToFp");
        if (valTy->isDoubleTy() && paramTy->isFloatTy())
            return m_builder.CreateFPTrunc(val, paramTy, "argFpTrunc");
        if (valTy->isFloatTy() && paramTy->isDoubleTy())
            return m_builder.CreateFPExt(val, paramTy, "argFpExt");
    }
    else if (paramTy->isIntegerTy() && valTy->isFloatingPointTy())
    {
        return m_builder.CreateFPToSI(val, paramTy, "argFpToInt");
    }
    else if (paramTy->isIntegerTy() && valTy->isIntegerTy() && valTy != paramTy)
    {
        if (valTy->getIntegerBitWidth() < paramTy->getIntegerBitWidth())
            return m_builder.CreateSExt(val, paramTy, "argIntExt");
        return m_builder.CreateTrunc(val, paramTy, "argIntTrunc");
    }

    return val;
}

llvm::Value *AbiClassifier::Decoerce(llvm::Value *coercedVal, llvm::StructType *structTy)
{
    llvm::AllocaInst* scratch = CreateEntryBlockAlloca("abiDecoerce", coercedVal->getType());
    m_builder.CreateStore(coercedVal, scratch);
    return m_builder.CreateLoad(structTy, scratch, "structResult");
}

llvm::AllocaInst *AbiClassifier::CreateEntryBlockAlloca(const std::string &name, llvm::Type *type)
{
    llvm::Function* func = m_builder.GetInsertBlock()->getParent();
    
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());

    return tmpBuilder.CreateAlloca(type, nullptr, name);
}
