#include "Compiler.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Helper.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

void Compiler::GenerateIR(std::shared_ptr<Node> rootNode, bool dumpIR)
{
    // Compile root node
    CompileNode(rootNode);

    if (dumpIR)
        module->print(llvm::outs(), nullptr);
}

void Compiler::EmitObjectFile(const std::string& filename)
{
    // Initialize targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    // Target triple
    std::string targetTriple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(llvm::Triple(targetTriple));

    // Lookup target
    std::string error;
    const llvm::Target* target =
        llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target)
    {
        llvm::errs() << "Target lookup failed: " << error << "\n";
        return;
    }

    // Create TargetMachine
    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);

    std::unique_ptr<llvm::TargetMachine> targetMachine(
        target->createTargetMachine(
            llvm::Triple(targetTriple),
            "generic",
            "",
            opt,
            RM
        )
    );

    // Apply DataLayout
    module->setDataLayout(targetMachine->createDataLayout());

    // Open output file
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);

    if (EC)
    {
        llvm::errs() << "Could not open file: " << EC.message() << "\n";
        return;
    }

    // Codegen pipeline
    llvm::legacy::PassManager pass;

    llvm::CodeGenFileType fileType = llvm::CodeGenFileType::ObjectFile;

    if (targetMachine->addPassesToEmitFile(
            pass,
            dest,
            nullptr,
            fileType))
    {
        llvm::errs() << "TargetMachine cannot emit this file type\n";
        return;
    }

    // Run passes
    pass.run(*module);

    dest.flush();
}

void Compiler::LinkObjectFile(const std::string &filepath)
{
    std::string linker = DetectLinker();

    if (linker.empty())
    {
        std::cerr << "No suitable linker found (clang++ or g++)\n";
        return;
    }

    std::string exeName = filepath + "program";

#ifdef _WIN32
    exeName += ".exe";
#endif

    std::string libPath = Helper::GetExecutableDir() + "/lib";

    std::string cmd =
        linker + " " +
        filepath + "output.o " +
        "-L\"" + libPath + "\" " +
        "-lruntime ";
    
    // append each natively linked library
    for (auto& [alias, libFilepath] : m_linkedLibs)
    {
        std::filesystem::path lp(libFilepath);
        std::string libDir = lp.parent_path().string();
 
        cmd += "\"" + libFilepath + "\" ";
#ifndef _WIN32
        // On Linux/macOS encode the library's directory into the binary's
        // rpath so dlopen/the dynamic linker finds it automatically.
        cmd += "-Wl,-rpath,\"" + libDir + "\" ";
#endif

    }

    cmd += "-o \"" + exeName + "\"";

    int result = std::system(cmd.c_str());

    if (result != 0)
        std::cerr << "Linking failed\n";
}

llvm::Value *Compiler::CompileNode(std::shared_ptr<Node> node)
{
    if (auto n = dynamic_cast<NumberNode*>(node.get()))
        return Compile_NumberNode(n);

    if (auto n = dynamic_cast<StringNode*>(node.get()))
        return Compile_StringNode(n);

    if (auto n = dynamic_cast<BinOpNode*>(node.get()))
        return Compile_BinOpNode(n);

    if (auto n = dynamic_cast<UnaryOpNode*>(node.get()))
        return Compile_UnaryOpNode(n);

    if (auto n = dynamic_cast<ListNode*>(node.get()))
        return Compile_ListNode(n);

    if (auto n = dynamic_cast<IndexGetNode*>(node.get()))
        return Compile_IndexGetNode(n);

    if (auto n = dynamic_cast<IndexAssignNode*>(node.get()))
        return Compile_IndexAssignNode(n);

    if (auto n = dynamic_cast<VarAccessNode*>(node.get()))
        return Compile_VarAccessNode(n);

    if (auto n = dynamic_cast<VarAssignNode*>(node.get()))
        return Compile_VarAssignNode(n);

    if (auto n = dynamic_cast<FuncDefNode*>(node.get()))
        return Compile_FuncDefNode(n);

    if (auto n = dynamic_cast<CallNode*>(node.get()))
        return Compile_CallNode(n);

    if (auto n = dynamic_cast<ReturnNode*>(node.get()))
        return Compile_ReturnNode(n);

    if (auto n = dynamic_cast<IfNode*>(node.get()))
        return Compile_IfNode(n);

    if (auto n = dynamic_cast<ForNode*>(node.get()))
        return Compile_ForNode(n);

    if (auto n = dynamic_cast<WhileNode*>(node.get()))
        return Compile_WhileNode(n);

    if (auto n = dynamic_cast<ContinueNode*>(node.get()))
        return Compile_ContinueNode(n);

    if (auto n = dynamic_cast<BreakNode*>(node.get()))
        return Compile_BreakNode(n);

    if (auto n = dynamic_cast<ModuleNode*>(node.get()))
        return Compile_ModuleNode(n);

    if (auto n = dynamic_cast<LinkNode*>(node.get()))
        return Compile_LinkNode(n);

    if (auto n = dynamic_cast<ExternNode*>(node.get()))
        return Compile_ExternNode(n);

    if (auto n = dynamic_cast<StructDefNode*>(node.get()))
        return Compile_StructDefNode(n);

    if (node.get() != nullptr)
        std::cerr << "Unknown node type '" << typeid(*node.get()).name() << "'\n";
    else
        std::cerr << "Node type to compile was null\n";
    return nullptr;
}

llvm::Value *Compiler::Compile_NumberNode(NumberNode* node)
{
    auto val = node->GetToken().GetValue();
    double num = 0;

    if (std::holds_alternative<int>(val))
        num = static_cast<double>(std::get<int>(val));
    else if (std::holds_alternative<double>(val))
        num = std::get<double>(val);

    return llvm::ConstantInt::get(builder.getInt32Ty(), num);
}

llvm::Value *Compiler::Compile_StringNode(StringNode *node)
{
    auto str = std::get<std::string>(node->GetToken().GetValue());

    return builder.CreateGlobalStringPtr(str, "strTmp");
}

llvm::Value *Compiler::Compile_ListNode(ListNode *node)
{
    llvm::Value* last = nullptr;

    for (auto& stmt : node->GetElementNodes())
        last = CompileNode(stmt);

    return last;
}

llvm::Value *Compiler::Compile_ArrayDeclaration(VarAssignNode *node, const std::string &name, ArrayTypeInfo info)
{
    auto* listNode = dynamic_cast<ListNode*>(node->GetValueNode().get());
    if (!listNode)
    {
        std::cerr << "Array variable '" << name << "' must be initialized with a list literal\n";
        return nullptr;
    }

    auto elementNodes = listNode->GetElementNodes();
    int count = (int)elementNodes.size();

    std::vector<llvm::Value*> compiledElems(count, nullptr);

    // infer the element type from the first element when none was declared
    if (!info.elementType)
    {
        if (count == 0)
        {
            std::cerr << "Cannot infer the type of an empty list literal for '" << name << "'; declare an explicit type, e.g. 'name : int[dyn] = []'\n";
            return nullptr;
        }

        compiledElems[0] = CompileNode(elementNodes[0]);
        if (!compiledElems[0])
            return nullptr;

        info.elementType = compiledElems[0]->getType();
        info.fixedSize = count;
    }

    if (!info.isDynamic && count != info.fixedSize)
    {
        std::cerr << "Array '" << name << "' declared with size " << info.fixedSize
                   << " but initializer has " << count << " element(s)\n";
        return nullptr;
    }

    int length = info.isDynamic ? count : info.fixedSize;

    VarInfo* existing = FindVariable(name);
    auto existingArrIt = m_arrays.find(name);
    bool existingIsArray = existing && existingArrIt != m_arrays.end();

    if (!node->GetIsDeclaration() && !existing)
    {
        std::cerr << "Variable '" << name << "' used before declaration; declare it with 'name : "
                   << (info.isDynamic ? (info.elementTypeName + "[dyn]") : (info.elementTypeName + "[" + std::to_string(length) + "]"))
                   << " = [...]'\n";
        return nullptr;
    }

    if (existing && !existingIsArray)
    {
        std::cerr << "Type mismatch: variable '" << name << "' is not an array\n";
        return nullptr;
    }

    llvm::AllocaInst* alloca = nullptr;
    bool isHeap = info.isDynamic;

    if (existingIsArray)
    {
        const ArrayInfo& oldInfo = existingArrIt->second;

        if (oldInfo.elementType != info.elementType || oldInfo.isDynamic != info.isDynamic)
        {
            std::cerr << "Type mismatch: array '" << name << "' cannot change element type or dyn/fixed-ness on reassignment\n";
            return nullptr;
        }

        if (!info.isDynamic)
        {
            if (oldInfo.length != length)
            {
                std::cerr << "Array '" << name << "' is a fixed size of " << oldInfo.length
                           << " and cannot be resized to " << length << " by reassignment\n";
                return nullptr;
            }

            alloca = existing->alloca;
        }
        else
        {
            // dyn array: free the old heap buffer, malloc a fresh one, rebind the pointer
            alloca = existing->alloca;

            llvm::Value* oldBase = builder.CreateLoad(llvm::PointerType::get(info.elementType, 0), alloca, name);
            llvm::Value* oldRaw = builder.CreateBitCast(oldBase, builder.getInt8Ty()->getPointerTo(), name + "OldRaw");
            builder.CreateCall(freeFunc, { oldRaw });
            m_heapValues.erase(oldBase);

            const llvm::DataLayout& dl = module->getDataLayout();
            uint64_t elemSize = dl.getTypeAllocSize(info.elementType);
            llvm::Value* sizeVal = llvm::ConstantInt::get(builder.getInt64Ty(), elemSize * (uint64_t)std::max(length, 0));

            llvm::Value* raw = builder.CreateCall(mallocFunc, { sizeVal }, name + "Raw");
            llvm::Value* basePtr = builder.CreateBitCast(raw, llvm::PointerType::get(info.elementType, 0), name + "Ptr");

            builder.CreateStore(basePtr, alloca);
            m_heapValues.insert(basePtr);
        }
    }
    else if (info.isDynamic)
    {
        // heap-allocate elementType[length] via malloc and store the base pointer in the alloca
        const llvm::DataLayout& dl = module->getDataLayout();
        uint64_t elemSize = dl.getTypeAllocSize(info.elementType);
        llvm::Value* sizeVal = llvm::ConstantInt::get(builder.getInt64Ty(), elemSize * (uint64_t)std::max(length, 0));

        llvm::Value* raw = builder.CreateCall(mallocFunc, { sizeVal }, name + "Raw");
        llvm::Value* basePtr = builder.CreateBitCast(raw, llvm::PointerType::get(info.elementType, 0), name + "Ptr");

        alloca = CreateEntryBlockAlloca(name, llvm::PointerType::get(info.elementType, 0));
        builder.CreateStore(basePtr, alloca);

        m_heapValues.insert(basePtr);
    }
    else
    {
        llvm::ArrayType* arrTy = llvm::ArrayType::get(info.elementType, (uint64_t)length);
        alloca = CreateEntryBlockAlloca(name, arrTy);
    }

    for (int i = 0; i < count; i++)
    {
        llvm::Value* elemVal = compiledElems[i] ? compiledElems[i] : CompileNode(elementNodes[i]);
        if (!elemVal)
            return nullptr;

        // widen/narrow integer literals to the declared element type
        if (elemVal->getType() != info.elementType && elemVal->getType()->isIntegerTy() && info.elementType->isIntegerTy())
            elemVal = builder.CreateIntCast(elemVal, info.elementType, true, "elemCast");

        llvm::Value* elemPtr = GetArrayElementPtr(alloca, info.elementType, length, info.isDynamic, llvm::ConstantInt::get(builder.getInt32Ty(), i), name);
        builder.CreateStore(elemVal, elemPtr);
    }

    if (!existingIsArray)
        SetVariable(name, { alloca, isHeap });

    m_arrays[name] = { info.elementType, info.isDynamic, length };

    return alloca;
}

llvm::Value *Compiler::Compile_IndexGetNode(IndexGetNode *node)
{
    auto* va = dynamic_cast<VarAccessNode*>(node->GetListNode().get());
    if (!va || va->GetIsNamespaced())
    {
        std::cerr << "Chained/nested indexing is not yet supported\n";
        return nullptr;
    }

    std::string name = std::get<std::string>(va->GetVarNameToken().GetValue());

    VarInfo* var = FindVariable(name);
    auto arrIt = m_arrays.find(name);
    if (!var || arrIt == m_arrays.end())
    {
        std::cerr << "Index access error: '" << name << "' is not a known array variable\n";
        return nullptr;
    }

    llvm::Value* indexVal = CompileNode(node->GetIndexNode());
    if (!indexVal)
        return nullptr;

    const ArrayInfo& info = arrIt->second;
    llvm::Value* elemPtr = GetArrayElementPtr(var->alloca, info.elementType, info.length, info.isDynamic, indexVal, name);

    return builder.CreateLoad(info.elementType, elemPtr, name + "Elem");
}

llvm::Value *Compiler::Compile_IndexAssignNode(IndexAssignNode *node)
{
    auto* va = dynamic_cast<VarAccessNode*>(node->GetListNode().get());
    if (!va || va->GetIsNamespaced())
    {
        std::cerr << "Chained/nested index assignment is not yet supported\n";
        return nullptr;
    }

    std::string name = std::get<std::string>(va->GetVarNameToken().GetValue());

    VarInfo* var = FindVariable(name);
    auto arrIt = m_arrays.find(name);
    if (!var || arrIt == m_arrays.end())
    {
        std::cerr << "Index assignment error: '" << name << "' is not a known array variable\n";
        return nullptr;
    }

    const ArrayInfo& info = arrIt->second;

    llvm::Value* indexVal = CompileNode(node->GetIndexNode());
    if (!indexVal)
        return nullptr;

    llvm::Value* value = CompileNode(node->GetValueNode());
    if (!value)
        return nullptr;

    if (value->getType() != info.elementType && value->getType()->isIntegerTy() && info.elementType->isIntegerTy())
        value = builder.CreateIntCast(value, info.elementType, true, "elemCast");

    if (value->getType() != info.elementType)
    {
        std::cerr << "Type mismatch assigning into array '" << name << "'\n";
        return nullptr;
    }

    llvm::Value* elemPtr = GetArrayElementPtr(var->alloca, info.elementType, info.length, info.isDynamic, indexVal, name);
    builder.CreateStore(value, elemPtr);

    return value;
}

llvm::Value *Compiler::Compile_BinOpNode(BinOpNode *node)
{
    llvm::Value* left = CompileNode(node->GetLeftNode());
    llvm::Value* right = CompileNode(node->GetRightNode());
    llvm::Type* leftTy = left->getType();
    llvm::Type* rightTy = right->getType();

    // types to string
    std::string leftTyStr;
    llvm::raw_string_ostream lrso(leftTyStr);
    leftTy->print(lrso);
    std::string rightTyStr;
    llvm::raw_string_ostream rrso(rightTyStr);
    rightTy->print(rrso);

    std::string op = node->GetOpToken().GetType();

    // Handle two numbers
    if ((leftTy->isDoubleTy() && rightTy->isDoubleTy()) || (leftTy->isIntegerTy() && rightTy->isIntegerTy()))
    {
        if (op == TT_PLUS)
            return builder.CreateAdd(left, right, "addTmp");
        if (op == TT_MINUS)
            return builder.CreateSub(left, right, "subTmp");
        if (op == TT_MUL)
            return builder.CreateMul(left, right, "mulTmp");
        if (op == TT_DIV)
            return builder.CreateSDiv(left, right, "divTmp"); // Signed
        if (op == TT_MOD)
            return builder.CreateSRem(left, right, "sremTmp"); // Signed
        if (op == TT_POW)
        {
            llvm::Function* powFunc = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::pow, {left->getType()});
            return builder.CreateCall(powFunc, {left, right}, "powTmp");
        }
        if (op == TT_EQEQ)
            return builder.CreateICmpEQ(left, right, "eqTmp");
        if (op == TT_NEQ)
            return builder.CreateICmpNE(left, right, "neTmp");
        if (op == TT_LT)
            return builder.CreateICmpSLT(left, right, "ltTmp"); // Signed
        if (op == TT_GT)
            return builder.CreateICmpSGT(left, right, "gtTmp"); // Signed
        if (op == TT_LTEQ)
            return builder.CreateICmpSLE(left, right, "lteTmp"); // Signed
        if (op == TT_GTEQ)
            return builder.CreateICmpSGE(left, right, "gteTmp"); // Signed
        if (node->GetOpToken().Matches(TT_KEYWORD, "AND"))
            return builder.CreateAnd(left, right, "andTmp");
        if (node->GetOpToken().Matches(TT_KEYWORD, "OR"))
            return builder.CreateOr(left, right, "orTmp");

        std::cerr << "Unsupported binary operation '" << op << "' for two numbers\n";
        return nullptr;
    }

    // Handle other types
    if (op == TT_PLUS)
    {
        llvm::Type* strTy = builder.getInt8Ty()->getPointerTo();

        // string + string
        if (leftTy == strTy && rightTy == strTy)
        {
            llvm::Value* val = builder.CreateCall(concatFunc, { left, right }, "strcatTmp");
            m_heapValues.insert(val);
            return val;
        }

        // string + int/double
        if (leftTy->isPointerTy() && (rightTy->isIntegerTy() || rightTy->isDoubleTy()))
        {
            llvm::Value* rightStr = IntToString(right);
            llvm::Value* val = builder.CreateCall(concatFunc, { left, rightStr}, "strcatTmp");
            m_heapValues.insert(val);
            return val;
        }

        //ToDo: List + ListVar

        std::cerr << "Unsupported binary operation '+' on type: '" << leftTyStr << "' and '" << rightTyStr << "'\n";
        return nullptr;
    }
    if (op == TT_MUL)
    {
        // ToDo: string * number
        // ToDo: number * string
        // ToDo: List * List
        
        std::cerr << "Unsupported binary operation 'MUL' on type: '" << leftTyStr << "' and '" << rightTyStr << "'\n";
        return nullptr;
    }

    std::cerr << "Unknown binary operation: '" << op << "'\n";
    return nullptr;
}

llvm::Value *Compiler::Compile_VarAccessNode(VarAccessNode *node)
{
    std::string name = std::get<std::string>(node->GetVarNameToken().GetValue());

    // compiler-builtin constants (true/false/null/math_pi/...) take priority over user variables, and are resolved before namespaced/user lookups.
    if (!node->GetIsNamespaced())
    {
        auto constIt = m_constants.find(name);
        if (constIt != m_constants.end())
            return constIt->second;
    }

    // namespaced access (module::variable)
    if (node->GetIsNamespaced())
    {
        const std::string namespaceName = node->GetNamespaceName().value();

        // struct fields access
        auto structIt = m_structs.varToType.find(namespaceName);
        if (structIt != m_structs.varToType.end())
        {
            const std::string& structTypeName = structIt->second;

            // find the field index by name
            auto fieldsIt = m_structs.fields.find(structTypeName);
            if (fieldsIt == m_structs.fields.end())
            {
                std::cerr << "VarAccess error: no fields for struct '" << structTypeName << "'\n";
                return nullptr;
            }

            const auto& fields = fieldsIt->second;
            int fieldIdx = -1;
            for (int i = 0; i < (int)fields.size(); ++i)
            {
                if (fields[i].attributeName == name)
                {
                    fieldIdx = i;
                    break;
                }
            }

            if (fieldIdx < 0)
            {
                std::cerr << "VarAccess error: Field '" << name << "' not found in struct '" << structTypeName << "'\n";
                return nullptr;
            }

            // load the struct* stored in the variable
            VarInfo* var = FindVariable(namespaceName);
            if (!var)
            {
                std::cerr << "VarAccess error: Struct variable '" << namespaceName << "' not found\n";
                return nullptr;
            }

            llvm::StructType* structTy = m_structs.types.at(structTypeName);
            llvm::Value* fieldPtr;

            if (var->alloca->getAllocatedType()->isStructTy())
                fieldPtr = builder.CreateStructGEP(structTy, var->alloca, (unsigned)fieldIdx, name + "Ptr");
            else
            {
                llvm::Value* structPtr = builder.CreateLoad(var->alloca->getAllocatedType(), var->alloca, namespaceName);
                fieldPtr = builder.CreateStructGEP(structTy, structPtr, (unsigned)fieldIdx, name + "Ptr");
            }

            llvm::Type* fieldTy = structTy->getElementType((unsigned)fieldIdx);

            return builder.CreateLoad(fieldTy, fieldPtr, name);
        }

        // module-variable access
        auto modIt = m_moduleVariables.find(namespaceName);
        if (modIt == m_moduleVariables.end())
        {
            std::cerr << "Module '" << namespaceName << "' not found\n";
            return nullptr;
        }

        auto varIt = modIt->second.find(name);
        if (varIt == modIt->second.end())
        {
            std::cerr << "'" << name << "' not found in module '" << namespaceName << "'\n";
            return nullptr;
        }

        const VarInfo& info = varIt->second;
        return builder.CreateLoad(info.alloca->getAllocatedType(), info.alloca, name);
    }

    // normal non-namespaced access
    VarInfo* var = FindVariable(name);
    if (!var)
    {
        std::cerr << "Undefined variable: " << name << "\n";
        return nullptr;
    }

    llvm::Value* val = builder.CreateLoad(var->alloca->getAllocatedType(), var->alloca, name);

    // When used as a plain value e.g. passed on to an extern that expects the struct by value, dereference the pointer to get the struct itself
    if (val->getType()->isPointerTy() && m_structs.IsStructVar(name))
    {
        const std::string& structTypeName = m_structs.varToType.at(name);
        llvm::StructType* structTy = m_structs.types.at(structTypeName);
        val = builder.CreateLoad(structTy, val, name + ".val");
    }

    return val;
}

llvm::Value *Compiler::Compile_VarAssignNode(VarAssignNode *node)
{
    std::string name = std::get<std::string>(node->GetVarNameToken().GetValue());

    // struct field assignment (e.g. red::r = 255)
    if (node->GetIsNamespaced())
    {
        const std::string& ownerName = node->GetNamespaceName().value();

        if (!m_structs.IsStructVar(ownerName))
        {
            std::cerr << "VarAssign error: '" << ownerName << "' is not a struct variable\n";
            return nullptr;
        }

        const std::string& structTypeName = m_structs.varToType.at(ownerName);
        auto fieldsIt = m_structs.fields.find(structTypeName);
        if (fieldsIt == m_structs.fields.end())
        {
            std::cerr << "VarAssign error: no fields for struct '" << structTypeName << "'\n";
            return nullptr;
        }

        const auto& fields = fieldsIt->second;
        int fieldIdx = -1;
        for (int i = 0; i < (int)fields.size(); ++i)
        {
            if (fields[i].attributeName == name)
            {
                fieldIdx = i;
                break;
            }
        }

        if (fieldIdx < 0)
        {
            std::cerr << "VarAssign error: field '" << name << "' not found in struct '" << structTypeName << "'\n";
            return nullptr;
        }

        llvm::Value* rhs = CompileNode(node->GetValueNode());
        if (!rhs)
            return nullptr;

        VarInfo* ownerVar = FindVariable(ownerName);
        if (!ownerVar)
        {
            std::cerr << "VarAssign error: variable '" << ownerName << "' not found\n";
            return nullptr;
        }

        llvm::StructType* structTy = m_structs.types.at(structTypeName);
        llvm::Value* fieldPtr;

        if (ownerVar->alloca->getAllocatedType()->isStructTy())
            fieldPtr = builder.CreateStructGEP(structTy, ownerVar->alloca, (unsigned)fieldIdx, name + "Ptr");
        else
        {
            llvm::Value* structPtr = builder.CreateLoad(ownerVar->alloca->getAllocatedType(), ownerVar->alloca, ownerName);
            fieldPtr =  builder.CreateStructGEP(structTy, structPtr, (unsigned)fieldIdx, name + "Ptr");
        }

        // widen an i32 RHS into whatever the field actually stores
        llvm::Type* fieldTy = structTy->getElementType((unsigned)fieldIdx);
        if (rhs->getType() != fieldTy && rhs->getType()->isIntegerTy() && fieldTy->isIntegerTy())
            rhs = builder.CreateTruncOrBitCast(rhs, fieldTy, "fieldCast");

        builder.CreateStore(rhs, fieldPtr);
        return rhs;
    }

    // array/list declaration with explicit type
    if (!node->GetIsNamespaced() && node->GetStrictVarDatatype().has_value())
    {
        auto arrayInfo = ParseArrayTypeName(node->GetStrictVarDatatype().value());
        if (arrayInfo.has_value())
            return Compile_ArrayDeclaration(node, name, arrayInfo.value());
    }

    // array/list declaration with an inferred type
    if (!node->GetIsNamespaced() && !node->GetStrictVarDatatype().has_value() && dynamic_cast<ListNode*>(node->GetValueNode().get()))
    {
        if (!node->GetIsDeclaration() && !FindVariable(name))
        {
            std::cerr << "Variable '" << name << "' used before declaration; declare it with 'name : var = [...]'\n";
            return nullptr;
        }

        ArrayTypeInfo inferredInfo;
        inferredInfo.elementType = nullptr;
        inferredInfo.isDynamic = false;
        inferredInfo.fixedSize = 0;

        return Compile_ArrayDeclaration(node, name, inferredInfo);
    }

    // struct instantiation (e.g. var red = Color)
    if (auto* varNode = dynamic_cast<VarAccessNode*>(node->GetValueNode().get()))
    {
        if (!varNode->GetIsNamespaced())
        {
            std::string typeName = std::get<std::string>(varNode->GetVarNameToken().GetValue());
            if (m_structs.HasType(typeName))
            {
                VarInfo* existing = FindVariable(name);

                if (!node->GetIsDeclaration() && !existing)
                {
                    std::cerr << "Variable '" << name << "' used before declaration; declare it with 'name : var = " << typeName << "'\n";
                    return nullptr;
                }
            
                llvm::StructType* structTy = m_structs.types.at(typeName);
                llvm::AllocaInst* alloca;
            
                if (existing)
                {
                    if (existing->alloca->getAllocatedType() != structTy)
                    {
                        std::cerr << "Type mismatch: variable '" << name << "' is not a '" << typeName << "'\n";
                        return nullptr;
                    }
                
                    alloca = existing->alloca;
                }
                else
                {
                    alloca = CreateEntryBlockAlloca(name, structTy);
                    SetVariable(name, { alloca, false });
                }
            
                builder.CreateStore(llvm::ConstantAggregateZero::get(structTy), alloca);
                m_structs.varToType[name] = typeName;
            
                return alloca;
            }
        }
    }

    // normal assignment
    llvm::Value* value = CompileNode(node->GetValueNode());
    if (!value) return nullptr;

    // enforce strict type annotation
    if (node->GetIsDeclaration() && node->GetStrictVarDatatype().has_value())
    {
        const std::string& declaredTypeName = node->GetStrictVarDatatype().value();
        llvm::Type* declaredTy = StringToLLVMType(declaredTypeName);

        if (!declaredTy)
        {
            std::cerr << "VarAssign error: unknown type '" << declaredTypeName << "' for variable '" << name << "'\n";
            return nullptr;
        }

        if (value->getType() != declaredTy)
        {
            std::cerr << "Type mismatch: variable '" << name << "' declared as '" << declaredTypeName << "' but initializer has a different type\n";
            return nullptr;
        }
    }

    // if RHS is a call to an extern that returns a struct, record the mapping
    if (value->getType()->isStructTy())
    {
        for (auto& [typeName, structTy] : m_structs.types)
        {
            if (structTy == value->getType())
            {
                m_structs.varToType[name] = typeName;
                break;
            }
        }
    }
    
    llvm::Type* type = value->getType();
    llvm::AllocaInst* alloca = nullptr;
    bool isHeap = m_heapValues.contains(value);

    VarInfo* existing = FindVariable(name);
    if (!existing)
    {
        if (!node->GetIsDeclaration())
        {
            std::cerr << "Variable '" << name << "' used before declaration; use 'name : type = expr' to declare it\n";
            return nullptr;
        }
        alloca = CreateEntryBlockAlloca(name, type);
        SetVariable(name, { alloca, isHeap });
    }
    else
        alloca = existing->alloca;

    if (alloca->getAllocatedType() != type)
    {
        std::cerr << "Type mismatch for variable: '" << name << "'\n";
        return nullptr;
    }

    builder.CreateStore(value, alloca);

    return value;
}

llvm::Value *Compiler::Compile_UnaryOpNode(UnaryOpNode *node)
{
    llvm::Value* val = CompileNode(node->GetNode());

    if (!val)
        return nullptr;

    std::string opToken = node->GetOpToken().GetType();
    llvm::Type* ty = val->getType();

    // INT
    if (ty->isIntegerTy())
    {
        if (opToken == TT_MINUS)
            return builder.CreateNeg(val, "negTmp");
        if (opToken == TT_PLUS)
            return val;

        if (node->GetOpToken().Matches(TT_KEYWORD, "not"))  // logical not
        {
            llvm::Value* cmp = builder.CreateICmpEQ(val, llvm::ConstantInt::get(ty, 0), "notTmp");

            // convert i1 to original integer type
            return builder.CreateIntCast(cmp, ty, false, "boolTmp");
        }

        std::cerr << "Unknown unary integer operator\n";
        return nullptr;
    }

    // FLOAT
    if (ty->isDoubleTy())
    {
        if (opToken == TT_MINUS)
            return builder.CreateFNeg(val, "fnegTmp");
        if (opToken == TT_PLUS)
            return val;

        if (node->GetOpToken().Matches(TT_KEYWORD, "not")) // logical not
        {
            llvm::Value* cmp = builder.CreateFCmpUEQ(val, llvm::ConstantFP::get(ty, 0.0), "fnotTmp");

            // bool to int32
            return builder.CreateIntCast(cmp, builder.getInt32Ty(), false, "boolTmp");
        }

        std::cerr << "Unknown unary float operator\n";
        return nullptr;
    }

    std::cerr << "Unsupported unary operator for type\n";
    return nullptr;
}

llvm::Value* Compiler::Compile_IfNode(IfNode* node, llvm::BasicBlock* existingMergeBB)
{
    llvm::Function* function = builder.GetInsertBlock()->getParent();

    // Reuse the caller's merge block if provided (elif/else-if chain)
    bool ownsMergeBB = (existingMergeBB == nullptr);
    llvm::BasicBlock* mergeBB = ownsMergeBB
        ? llvm::BasicBlock::Create(context, "ifcont")
        : existingMergeBB;

    llvm::BasicBlock* nextCondBB = nullptr;

    for (size_t i = 0; i < node->GetCases().size(); i++)
    {
        auto& ifCase = node->GetCases()[i];

        if (ifCase.GetCondition() == nullptr)
        {
            CompileNode(ifCase.GetExpr());
            builder.CreateBr(mergeBB);
            break;
        }

        llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(context, "then", function);

        llvm::Value* cond = CompileNode(ifCase.GetCondition());
        cond = builder.CreateICmpNE(
            cond,
            llvm::ConstantInt::get(cond->getType(), 0),
            "ifcond"
        );

        bool isLastCase = (i == node->GetCases().size() - 1);
        if (isLastCase && !node->GetElseCase())
            nextCondBB = mergeBB;
        else
            nextCondBB = llvm::BasicBlock::Create(context, "else");

        builder.CreateCondBr(cond, thenBB, nextCondBB);

        builder.SetInsertPoint(thenBB);
        CompileNode(ifCase.GetExpr());
        builder.CreateBr(mergeBB);

        if (nextCondBB != mergeBB)
        {
            function->insert(function->end(), nextCondBB);
            builder.SetInsertPoint(nextCondBB);
        }
    }

    if (node->GetElseCase())
    {
        // If the else body is itself an if-chain, pass mergeBB down so it does not create a redundant ifcont of its own
        if (auto* elseIf = dynamic_cast<IfNode*>(node->GetElseCase().get()))
            Compile_IfNode(elseIf, mergeBB);
        else
        {
            CompileNode(node->GetElseCase());
            builder.CreateBr(mergeBB);
        }
    }

    // Only insert+own the block if we created it
    if (ownsMergeBB)
        function->insert(function->end(), mergeBB);

    builder.SetInsertPoint(mergeBB);
    return nullptr;
}

llvm::Value *Compiler::Compile_ForNode(ForNode *node)
{
    llvm::Function* function = builder.GetInsertBlock()->getParent();

    PushScope();

    std::string varName = std::get<std::string>(node->GetVarNameTok().GetValue());

    llvm::Value* startVal = CompileNode(node->GetStartValueNode());

    llvm::AllocaInst* alloca = CreateEntryBlockAlloca(varName, startVal->getType());
    builder.CreateStore(startVal, alloca);

    SetVariable(varName, { alloca, false });

    // Blocks
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(context, "for.cond", function);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(context, "for.body", function);
    llvm::BasicBlock* stepBB = llvm::BasicBlock::Create(context, "for.step", function);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(context, "for.end", function);

    // Push loop context
    m_loopStack.push_back({ stepBB, afterBB });

    builder.CreateBr(condBB);

    // condition
    builder.SetInsertPoint(condBB);

    llvm::Value* currentVal = builder.CreateLoad(
        alloca->getAllocatedType(), alloca, varName);

    llvm::Value* endVal = CompileNode(node->GetEndValueNode());

    // get step for condition
    bool negativeStep = false;
    if (node->GetStepValueNode())
    {
        if (auto num = dynamic_cast<NumberNode*>(node->GetStepValueNode().get()))
        {
            auto val = num->GetToken().GetValue();

            if (std::holds_alternative<int>(val))
            {
                if (std::get<int>(val) == 0)
                {
                    std::cerr << "For loop step cannot be 0\n"; // would create infinite loop
                    return nullptr;
                }

                negativeStep = std::get<int>(val) < 0;
            }
        }
    }

    // build condition
    llvm::Value* cond;
    if (!negativeStep)
        cond = builder.CreateICmpSLE(currentVal, endVal);
    else
        cond = builder.CreateICmpSGE(currentVal, endVal);

    builder.CreateCondBr(cond, bodyBB, afterBB);

    // body
    builder.SetInsertPoint(bodyBB);

    CompileNode(node->GetBodyNode());

    // If body didn't already terminate (break/return)
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(stepBB);

    // step
    builder.SetInsertPoint(stepBB);

    llvm::Value* stepVal = node->GetStepValueNode()
        ? CompileNode(node->GetStepValueNode())
        : llvm::ConstantInt::get(builder.getInt32Ty(), 1);

    currentVal = builder.CreateLoad(alloca->getAllocatedType(), alloca, varName);
    llvm::Value* nextVal = builder.CreateAdd(currentVal, stepVal);

    builder.CreateStore(nextVal, alloca);
    builder.CreateBr(condBB);

    // after
    builder.SetInsertPoint(afterBB);

    m_loopStack.pop_back();
    PopScope();

    return nullptr;
}

llvm::Value *Compiler::Compile_WhileNode(WhileNode *node)
{
    llvm::Function* function = builder.GetInsertBlock()->getParent();

    PushScope();

    // Blocks
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(context, "while.cond", function);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(context, "while.body", function);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(context, "while.end", function);

    // continue -> recheck condition
    // break -> exit loop
    m_loopStack.push_back({ condBB, afterBB });

    // Initial jump to condition
    builder.CreateBr(condBB);

    // condition
    builder.SetInsertPoint(condBB);

    llvm::Value* condVal = CompileNode(node->GetConditionNode());
    if (!condVal)
        return nullptr;

    llvm::Value* cond;

    // INT condition
    if (condVal->getType()->isIntegerTy())
        cond = builder.CreateICmpNE(condVal, llvm::ConstantInt::get(condVal->getType(), 0), "whilecond");
    else if (condVal->getType()->isDoubleTy())
        cond = builder.CreateFCmpONE(condVal, llvm::ConstantFP::get(condVal->getType(), 0.0), "whilecond");
    else
    {
        std::cerr << "Invalid while condition type\n";
        return nullptr;
    }

    builder.CreateCondBr(cond, bodyBB, afterBB);

    // body
    builder.SetInsertPoint(bodyBB);

    CompileNode(node->GetBodyNode());

    // if body didn't already terminate
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(condBB);

    // after
    builder.SetInsertPoint(afterBB);

    m_loopStack.pop_back();

    FreeLocalHeapValues();
    PopScope();

    return nullptr;
}

llvm::Value *Compiler::Compile_FuncDefNode(FuncDefNode *node)
{
    if (!node->GetVarNameTok().has_value())
    {
        std::cerr << "Anonymous functions not supported yet!\n";
        return nullptr;
    }

    std::string name = std::get<std::string>(node->GetVarNameTok()->GetValue());

    llvm::Type* returnType = InferenceReturnType(node->GetBodyNode(), node->GetShouldAutoReturn());

    // scan the body for "param::field" usage to detect struct params
    std::vector<std::string> structParamTypes; // parallel to ArgNameToks; "" = not a struct
    std::vector<llvm::Type*> argTypes;

    for (auto& argTok : node->ArgNameToks())
    {
        std::string paramName = std::get<std::string>(argTok.argNameTok.GetValue());
        std::string structTypeName = ScanForStructParamUsage(paramName, node->GetBodyNode());

        structParamTypes.push_back(structTypeName);

        if (!structTypeName.empty())
            argTypes.push_back(llvm::PointerType::get(m_structs.types.at(structTypeName), 0));
        else
            argTypes.push_back(builder.getInt32Ty());
    }

    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, argTypes, false);

    llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, module.get());

    m_functions[name] = func;

    // Name arguments
    int idx = 0;
    for (auto& arg : func->args())
    {
        arg.setName(std::get<std::string>(node->ArgNameToks()[idx].argNameTok.GetValue()));
        idx++;
    }

    // Create entry
    llvm::BasicBlock* block = llvm::BasicBlock::Create(context, "entry", func);

    builder.SetInsertPoint(block);

    // New variable scope
    PushScope();

    // Store and allocate variables
    {
        size_t i = 0;
        for (auto& arg : func->args())
        {
            llvm::AllocaInst* alloca = CreateEntryBlockAlloca(arg.getName().str(), arg.getType());
            builder.CreateStore(&arg, alloca);
            SetVariable(arg.getName().str(), { alloca, false });

            // register struct-ptr params so param::field resolves during body compilation
            if (i < structParamTypes.size() && ! structParamTypes[i].empty())
                m_structs.varToType[arg.getName().str()] = structParamTypes[i];

            ++i;
        }
    }

    // Compile body
    llvm::Value* retVal = CompileNode(node->GetBodyNode());

    // Free all localy created heap values at end of function
    FreeLocalHeapValues();

    PopScope();

    if (node->GetShouldAutoReturn() && retVal)
        builder.CreateRet(retVal);
    else
    {
        llvm::BasicBlock* currentBB = builder.GetInsertBlock();
        if (!currentBB->getTerminator())
        {
            // build a type-correct fallback return value (ptr null / i32 0)
            llvm::Value* defaultRet;
            if (returnType->isPointerTy())
                defaultRet = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(returnType));
            else
                defaultRet = llvm::ConstantInt::get(returnType, 0);

            builder.CreateRet(defaultRet);
        }
    }

    llvm::verifyFunction(*func);

    return func;
}

llvm::Value *Compiler::Compile_CallNode(CallNode *node)
{
    auto varAccess = dynamic_cast<VarAccessNode*>(node->GetNodeToCall().get());
    if (!varAccess)
    {
        std::cerr << "Invalid function call!\n";
        return nullptr;
    }

    std::string name = std::get<std::string>(varAccess->GetVarNameToken().GetValue());

    // namespaced call (Module::function(args))
    if (varAccess->GetIsNamespaced())
    {
        const std::string namespaceName = varAccess->GetNamespaceName().value();

        // call function from extern .dll or .so file
        auto externModIt = m_externFunctions.find(namespaceName);
        if (externModIt != m_externFunctions.end())
        {
            auto externFuncIt = externModIt->second.find(name);
            if (externFuncIt != externModIt->second.end())
            {
                llvm::Function* externFunc = externFuncIt->second;
                llvm::FunctionType* externFunctTy = externFunc->getFunctionType();

                std::vector<llvm::Value*> args;
                for (size_t i = 0; i < node->GetArgNodes().size(); ++i)
                {
                    llvm::Value* argVal = CompileNode(node->GetArgNodes()[i]);
                    if (!argVal)
                        return nullptr;

                    // C ABI structs are passed by value.
                    // argVal is already a struct value (Compile_VarAccessNode loads it from its alloca), so no conversion needed.
                    args.push_back(argVal);
                }

                if (externFunctTy->getReturnType()->isVoidTy())
                {
                    builder.CreateCall(externFunc, args);
                    return llvm::ConstantInt::get(builder.getInt32Ty(), 0);
                }
                return builder.CreateCall(externFunc, args, "externCallTmp");
            }
        }

        // call function in separate module
        auto modIt = m_moduleFunctions.find(namespaceName);
        if (modIt == m_moduleFunctions.end())
        {
            std::cerr << "Module '" << namespaceName << "' not found\n";
            return nullptr;
        }

        auto funcIt = modIt->second.find(name);
        if (funcIt == modIt->second.end())
        {
            std::cerr << "Function '" << name << "' not found in module '" << namespaceName << "'\n";
            return nullptr;
        }

        llvm::Function* func = funcIt->second;

        std::vector<llvm::Value*> args;
        for (auto& argNode : node->GetArgNodes())
        {
            // mirrors the same by-reference logic as the local user-function call path below, so struct args behave consistently
            if (auto* va = dynamic_cast<VarAccessNode*>(argNode.get()); va && !va->GetIsNamespaced())
            {
                const std::string& varName = std::get<std::string>(va->GetVarNameToken().GetValue());
                VarInfo* var = FindVariable(varName);
                if (var)
                {
                    if (var->alloca->getAllocatedType()->isStructTy())
                    {
                        args.push_back(var->alloca);
                        continue;
                    }
                    if (var->alloca->getAllocatedType()->isPointerTy() && m_structs.IsStructVar(varName))
                    {
                        llvm::Value* ptrVal = builder.CreateLoad(var->alloca->getAllocatedType(), var->alloca, varName);
                        args.push_back(ptrVal);
                        continue;
                    }
                }
            }

            llvm::Value* argVal = CompileNode(argNode);
            if (!argVal)
                return nullptr;

            if (argVal->getType()->isStructTy())
            {
                llvm::AllocaInst* tmp = CreateEntryBlockAlloca("structArg", argVal->getType());
                builder.CreateStore(argVal, tmp);
                argVal = tmp;
            }

            args.push_back(argVal);
        }

        return builder.CreateCall(func, args, "callTmp");
    }

    // Builtin functions
    if (m_builtins.find(name) != m_builtins.end())
    {
        std::vector<llvm::Value*> args;

        for (auto& argNode : node->GetArgNodes())
            args.push_back(CompileNode(argNode));

        return m_builtins[name]->Codegen(builder, args);
    }

    // Normal functions (user-defined)
    if (m_functions.find(name) == m_functions.end())
    {
        std::cerr << "Unknown function: '" << name << "'!\n";
        return nullptr;
    }

    llvm::Function* func = m_functions[name];

    std::vector<llvm::Value*> args;
    for (auto& argNode : node->GetArgNodes())
    {
        // if the argument is a plain variable that already holds a struct, pass the address of its *existing* alloca directly
        if (auto* va = dynamic_cast<VarAccessNode*>(argNode.get()); va && !va->GetIsNamespaced())
        {
            const std::string& varName = std::get<std::string>(va->GetVarNameToken().GetValue());
            VarInfo* var = FindVariable(varName);
            if (var)
            {
                if (var->alloca->getAllocatedType()->isStructTy())
                {
                    // struct stored directly -> pass the address of the real storage
                    args.push_back(var->alloca);
                    continue;
                }
                if (var->alloca->getAllocatedType()->isPointerTy() && m_structs.IsStructVar(varName))
                {
                    // already a struct pointer (e.g. forwarding a param) -> pass it through as-is
                    llvm::Value* ptrVal = builder.CreateLoad(var->alloca->getAllocatedType(), var->alloca, varName);
                    args.push_back(ptrVal);
                    continue;
                }
            }
        }

        llvm::Value* argVal = CompileNode(argNode);
        if (!argVal)
            return nullptr;

        // non-variable expression producing a struct value (e.g. a call that returns a struct) has no original storage to alias, so a temp is the only option here
        if (argVal->getType()->isStructTy())
        {
            llvm::AllocaInst* tmp = CreateEntryBlockAlloca("structArg", argVal->getType());
            builder.CreateStore(argVal, tmp);
            argVal = tmp;
        }

        args.push_back(argVal);
    }

    return builder.CreateCall(func, args, "callTmp");
}

llvm::Value *Compiler::Compile_ReturnNode(ReturnNode *node)
{
    llvm::Value* val;

    if (node->GetNodeToReturn().has_value())
        val = CompileNode(node->GetNodeToReturn().value());
    else
        val = llvm::ConstantInt::get(builder.getInt32Ty(), 0);

    return builder.CreateRet(val);
}

llvm::Value *Compiler::Compile_ContinueNode(ContinueNode *node)
{
    if (m_loopStack.empty())
    {
        std::cerr << "Continue outside loop\n";
        return nullptr;
    }

    builder.CreateBr(m_loopStack.back().continueBB);

    // Create dead block so IR stays valid
    llvm::BasicBlock* deadBB = llvm::BasicBlock::Create(context, "aftercontinue", builder.GetInsertBlock()->getParent());
    builder.SetInsertPoint(deadBB);

    return nullptr;
}

llvm::Value *Compiler::Compile_BreakNode(BreakNode *node)
{
    if (m_loopStack.empty())
    {
        std::cerr << "Break outside loop\n";
        return nullptr;
    }

    builder.CreateBr(m_loopStack.back().breakBB);

    // Create dead block so IR stays valid
    llvm::BasicBlock* deadBB = llvm::BasicBlock::Create(context, "afterbreak", builder.GetInsertBlock()->getParent());
    builder.SetInsertPoint(deadBB);

    return nullptr;
}

llvm::Value *Compiler::Compile_ModuleNode(ModuleNode *node)
{
    std::filesystem::path filepath(std::get<std::string>(node->GetFilepathToken().GetValue()));

    // if path is relative, resolve it based on importing file's directory
    if (!filepath.is_absolute())
    {
        std::filesystem::path base(m_mainFilepath);
        filepath = base.parent_path() / filepath;
    }

    if (!std::filesystem::exists(filepath))
    {
        std::cerr << "Module not found: " << filepath.string() << "\n";
        return nullptr;
    }

    // normalize (e.g. resolve "..", ".")
    filepath = std::filesystem::canonical(filepath);

    // read source
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "Could not open module file: " << filepath.string() << "\n";
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string fileContent = buffer.str();
    file.close();

    // lex source
    Lexer lexer(filepath.string(), fileContent);
    auto lexResult = lexer.MakeTokens();
    if (lexResult.error != nullptr)
    {
        std::cerr << "Lexer error in module!\n" << Helper::GetErrorString(lexResult.error.get()) << "\n";
        return nullptr;
    }

    // parse lexResult
    Parser parser(lexResult.tokens);
    auto parseResult = parser.Parse();
    if (parseResult.HasError())
    {
        std::cerr << "Parser error in module!\n" << Helper::GetErrorString(parseResult.GetErrorPtr()) << "\n";
        return nullptr;
    }

    const std::string alias = node->GetAlias();

    // snapeshot what already exists so we know what the module adds
    auto functionsBefore = m_functions;
    auto variablesBefore = CollectAllVariables();

    // swap in the module's filepath so any nested #module directives inside this file resolve relative to *it*, not the importer
    std::string savedFilepath = m_mainFilepath;
    m_mainFilepath = filepath.string();

    CompileNode(parseResult.GetNode());

    // record functions introduced by the module
    for (auto& [name, func] : m_functions)
    {
        if (functionsBefore.find(name) == functionsBefore.end())
            m_moduleFunctions[alias][name] = func;
    }

    // record module-level variables (globals / top-level allocas)
    for (auto& [name, info] : CollectAllVariables())
    {
        if (variablesBefore.find(name) == variablesBefore.end())
            m_moduleVariables[alias][name] = info;
    }

    return nullptr;
}

llvm::Value *Compiler::Compile_LinkNode(LinkNode *node)
{
    std::filesystem::path filepath(std::get<std::string>(node->GetFilepathToken().GetValue()));

    // resolve relative paths against the current source file
    if (!filepath.is_absolute())
    {
        std::filesystem::path base(m_mainFilepath);
        filepath = base.parent_path() / filepath;
    }

    if (!std::filesystem::exists(filepath))
    {
        std::cerr << "Link file not found: '" << filepath.string() << "'\n";
        return nullptr;
    }

    // normalize
    filepath = std::filesystem::canonical(filepath);

    m_linkedLibs[node->GetAlias()] = filepath.string();

    return nullptr;
}

llvm::Value *Compiler::Compile_ExternNode(ExternNode *node)
{
    const std::string& moduleAlias = node->GetModuleAlias();
    const std::string& functionName = node->GetFunctionName();
    const std::string& signature = node->GetSignature();

    // validate that the library was linked first
    if (m_linkedLibs.find(moduleAlias) == m_linkedLibs.end())
    {
        std::cerr << "Extern error: linked module alias '" << moduleAlias << "' not found. Did you forgot '#link \"lib.so\" as " << moduleAlias << "'\n";
        return nullptr;
    }

    // parse signature "returnType(arg1, arg2, ...)"
    auto openParen = signature.find('(');
    auto closeParen = signature.find(')');

    if (openParen == std::string::npos || closeParen == std::string::npos || closeParen < openParen)
    {
        std::cerr << "Extern error: invalid signature '" << signature << "' for function '" << functionName << "'\n";
        return nullptr;
    }

    std::string returnTypeString = signature.substr(0, openParen);
    std::string argsString = signature.substr(openParen + 1, closeParen - openParen - 1);

    // trim whitespace from the return type string
    auto trimStr = [](std::string& s) { s.erase(0, s.find_first_not_of(" \t")); s.erase(s.find_last_not_of(" \t") + 1); };
    trimStr(returnTypeString);

    // build llvm type lists
    llvm::Type* returnType;
    if (m_structs.HasType(returnTypeString))
        returnType = m_structs.types.at(returnTypeString);
    else
        returnType = StringToLLVMType(returnTypeString);

    std::vector<llvm::Type*> argTypes;
    if (!argsString.empty())
    {
        std::stringstream ss(argsString);
        std::string typeToken;
        while (std::getline(ss, typeToken, ','))
        {
            trimStr(typeToken);
            // Extern functions follow C ABI -> structs are passed by value, so map a struct name to its struct type, not a pointer
            llvm::Type* argTy = m_structs.HasType(typeToken) ? static_cast<llvm::Type*>(m_structs.types.at(typeToken)) : StringToLLVMType(typeToken);
            argTypes.push_back(argTy);
        }
    }

    // declare or reuse the function in the LLVM module. if the same native function is declared twice we reuse the existing declaration rather than creating a duplicate
    llvm::Function* func = module->getFunction(functionName);
    if (!func)
    {
        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, argTypes, /*isVarArg=*/false);

        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, functionName, module.get());
    }
    else
    {
        // sanity-check
        llvm::FunctionType* existing = func->getFunctionType();
        if (existing->getReturnType() != returnType || existing->getNumParams() != static_cast<unsigned>(argTypes.size()))
        {
            std::cerr << "Extern error: conflicting declarations for '" << functionName << "'\n";
            return nullptr;
        }
    }

    m_externFunctions[moduleAlias][functionName] = func;
    m_externReturnTypeStrings[moduleAlias][functionName] = returnTypeString;

    return nullptr;
}

llvm::Value *Compiler::Compile_StructDefNode(StructDefNode *node)
{
    std::string structName = std::get<std::string>(node->GetVarNameTok().GetValue());

    // avoid reregistering the same struct type
    if (m_structs.HasType(structName))
    {
        std::cerr << "Struct warning: '" << structName << "' is already defined; Ignoring redefinition\n";
        return nullptr;
    }

    // build the ordered list of llvm field types
    std::vector<llvm::Type*> fieldTypes;
    for (const auto& attr : node->GetAttributeToks())
        fieldTypes.push_back(StringToLLVMType(attr.attributeType));

    // create a named (non-opaque) struct type in this context
    llvm::StructType* structType = llvm::StructType::create(context, fieldTypes, structName);

    m_structs.types[structName] = structType;
    m_structs.fields[structName] = node->GetAttributeToks();

    return nullptr;
}

void Compiler::PushScope()
{
    m_scopes.emplace_back();
}

void Compiler::PopScope()
{
    m_scopes.pop_back();
}

VarInfo *Compiler::FindVariable(const std::string &name)
{
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
    {
        if (it->contains(name))
            return &(*it)[name];
    }
    return nullptr;
}

void Compiler::SetVariable(const std::string &name, VarInfo info)
{
    m_scopes.back()[name] = info;
}

std::unordered_map<std::string, VarInfo> Compiler::CollectAllVariables() const
{
    std::unordered_map<std::string, VarInfo> result;
    for (const auto& scope : m_scopes)
    {
        for (const auto& [name, info] : scope)
        {
            result[name] = info;
        }
    }
    
    return result;
}

void Compiler::FreeLocalHeapValues()
{
    if (m_scopes.empty())
        return;

    auto& scope = m_scopes.back();

    for (auto& [name, info] : scope)
    {
        if (!info.isHeapAllocated)
            continue;

        llvm::Value* loaded = builder.CreateLoad(info.alloca->getAllocatedType(), info.alloca, name);

        builder.CreateCall(freeFunc, { loaded });
    }
}

std::string Compiler::ScanForStructParamUsage(const std::string &paramName, std::shared_ptr<Node> body)
{
    if (!body)
        return "";

    // paramName::field (read access)
    if (auto* va = dynamic_cast<VarAccessNode*>(body.get()))
    {
        if (va->GetIsNamespaced() && va->GetNamespaceName().value() == paramName)
        {
            std::string fieldName = std::get<std::string>(va->GetVarNameToken().GetValue());
            for (auto& [typeName, fields] : m_structs.fields)
            {
                for (auto& field : fields)
                {
                    if (field.attributeName == fieldName)
                        return typeName;
                }
            }
        }
        return "";
    }

    // paramName::field = value (write access)
    if (auto* assign = dynamic_cast<VarAssignNode*>(body.get()))
    {
        if (assign->GetIsNamespaced() && assign->GetNamespaceName().value() == paramName)
        {
            std::string fieldName = std::get<std::string>(assign->GetVarNameToken().GetValue());
            for (auto& [typeName, fields] : m_structs.fields)
            {
                for (auto& field : fields)
                {
                    if (field.attributeName == fieldName)
                        return typeName;
                }
            }
        }
        if (auto r = ScanForStructParamUsage(paramName, assign->GetValueNode()); !r.empty())
            return r;
        return "";
    }

    // --- recurse into child nodes ---

    if (auto* list = dynamic_cast<ListNode*>(body.get()))
    {
        for (auto& elem : list->GetElementNodes())
            if (auto r = ScanForStructParamUsage(paramName, elem); !r.empty())
                return r;
        return "";
    }

    if (auto* call = dynamic_cast<CallNode*>(body.get()))
    {
        for (auto& arg : call->GetArgNodes())
            if (auto r = ScanForStructParamUsage(paramName, arg); !r.empty())
                return r;
        return "";
    }

    if (auto* bin = dynamic_cast<BinOpNode*>(body.get()))
    {
        if (auto r = ScanForStructParamUsage(paramName, bin->GetLeftNode());  !r.empty()) return r;
        if (auto r = ScanForStructParamUsage(paramName, bin->GetRightNode()); !r.empty()) return r;
        return "";
    }

    if (auto* un = dynamic_cast<UnaryOpNode*>(body.get()))
        return ScanForStructParamUsage(paramName, un->GetNode());

    if (auto* ret = dynamic_cast<ReturnNode*>(body.get()))
        return ret->GetNodeToReturn().has_value()
                   ? ScanForStructParamUsage(paramName, ret->GetNodeToReturn().value())
                   : "";

    if (auto* ifNode = dynamic_cast<IfNode*>(body.get()))
    {
        for (auto& c : ifNode->GetCases())
            if (auto r = ScanForStructParamUsage(paramName, c.GetExpr()); !r.empty())
                return r;
        if (ifNode->GetElseCase())
            return ScanForStructParamUsage(paramName, ifNode->GetElseCase());
        return "";
    }

    if (auto* whileNode = dynamic_cast<WhileNode*>(body.get()))
    {
        if (auto r = ScanForStructParamUsage(paramName, whileNode->GetConditionNode()); !r.empty())
            return r;
        return ScanForStructParamUsage(paramName, whileNode->GetBodyNode());
    }

    if (auto* forNode = dynamic_cast<ForNode*>(body.get()))
        return ScanForStructParamUsage(paramName, forNode->GetBodyNode());

    return "";
}

// returns the LLVM type a single expression will produce. 'locals' carries variable types tracked by InferReturnTypeBlock
llvm::Type *Compiler::InferenceExprType(std::shared_ptr<Node> node, const LocalTypeMap& locals)
{
    if (!node)
        return builder.getInt32Ty();

    // if literals
    if (dynamic_cast<NumberNode*>(node.get()))
        return builder.getInt32Ty();

    if (dynamic_cast<StringNode*>(node.get()))
        return llvm::PointerType::get(builder.getInt8Ty(), 0);

    // ListNode (array literals)
    if (dynamic_cast<ListNode*>(node.get()))
        return llvm::PointerType::get(builder.getInt8Ty(), 0);

    // unary operation (same type as operand)
    if (auto u = dynamic_cast<UnaryOpNode*>(node.get()))
        return InferenceExprType(u->GetNode(), locals);

    // binary operation (ptr if either side is ptr)
    if (auto b = dynamic_cast<BinOpNode*>(node.get()))
    {
        llvm::Type* L = InferenceExprType(b->GetLeftNode(),  locals);
        llvm::Type* R = InferenceExprType(b->GetRightNode(), locals);
        if (L->isPointerTy() || R->isPointerTy())
            return llvm::PointerType::get(builder.getInt8Ty(), 0);
        else
            return builder.getInt32Ty();
    }

    // variable access
    if (auto va = dynamic_cast<VarAccessNode*>(node.get()))
    {
        std::string name = std::get<std::string>(va->GetVarNameToken().GetValue());

        // 0. compiler constants
        if (!va->GetIsNamespaced())
        {
            auto constantIt = m_constants.find(name);
            if (constantIt != m_constants.end())
                return constantIt->second->getType();
        }
 
        // 1. pre-pass local map (variables assigned above this point in the body)
        auto it = locals.find(name);
        if (it != locals.end())
            return it->second;
 
        // 2. already-compiled outer scope allocas (e.g. captured from enclosing func)
        VarInfo* var = FindVariable(name);
        if (var)
            return var->alloca->getAllocatedType();
 
        return builder.getInt32Ty();
    }

    // function call: use the callee's declared return type
    if (auto call = dynamic_cast<CallNode*>(node.get()))
    {
        if (auto va = dynamic_cast<VarAccessNode*>(call->GetNodeToCall().get()))
        {
            std::string name = std::get<std::string>(va->GetVarNameToken().GetValue());
            auto it = m_functions.find(name);
            if (it != m_functions.end())
                return it->second->getReturnType();
        }
        return builder.getInt32Ty(); // forward/unknown call → assume i32
    }
 
    return builder.getInt32Ty();
}

//  Arrow functions (autoReturn=true):  body IS the expression.
//  Block functions (autoReturn=false): walk statements for return.
llvm::Type *Compiler::InferenceReturnType(std::shared_ptr<Node> body, bool autoReturn)
{
    if (autoReturn)
        return InferenceExprType(body, {}); // body = single expr, no locals needed
 
    LocalTypeMap locals;
    return InferenceReturnTypeBlock(body, locals);
}

// walks the body of a block function. Tracks VarAssignNodes into 'locals' so subsequent expression inference can resolve variable types without compiled allocas
llvm::Type *Compiler::InferenceReturnTypeBlock(std::shared_ptr<Node> node, LocalTypeMap& locals)
{
    if (!node)
        return builder.getInt32Ty();
 
    // explicit return statement
    if (auto ret = dynamic_cast<ReturnNode*>(node.get()))
    {
        if (ret->GetNodeToReturn().has_value())
            return InferenceExprType(ret->GetNodeToReturn().value(), locals);
        return builder.getInt32Ty();
    }
 
    // statement list (ListNode used as block body)
    if (auto list = dynamic_cast<ListNode*>(node.get()))
    {
        for (auto& stmt : list->GetElementNodes())
        {
            // track variable assignments so later stmts can use the type
            if (auto assign = dynamic_cast<VarAssignNode*>(stmt.get()))
            {
                std::string varName = std::get<std::string>(assign->GetVarNameToken().GetValue());
                locals[varName] = InferenceExprType(assign->GetValueNode(), locals);
                continue;
            }
 
            // explicit return found
            if (auto ret = dynamic_cast<ReturnNode*>(stmt.get()))
            {
                if (ret->GetNodeToReturn().has_value())
                    return InferenceExprType(ret->GetNodeToReturn().value(), locals);
                return builder.getInt32Ty();
            }
 
            // recurse into nested control flow
            llvm::Type* t = InferenceReturnTypeBlock(stmt, locals);
            if (t->isPointerTy())
                return t;
        }
 
        return builder.getInt32Ty(); // no explicit return found
    }
 
    // control flow: recurse into branches
    if (auto ifNode = dynamic_cast<IfNode*>(node.get()))
    {
        for (auto& c : ifNode->GetCases())
        {
            llvm::Type* t = InferenceReturnTypeBlock(c.GetExpr(), locals);
            if (t->isPointerTy()) return t;
        }
        if (ifNode->GetElseCase())
            return InferenceReturnTypeBlock(ifNode->GetElseCase(), locals);
        return builder.getInt32Ty();
    }
 
    if (auto forNode = dynamic_cast<ForNode*>(node.get()))
        return InferenceReturnTypeBlock(forNode->GetBodyNode(), locals);
 
    if (auto whileNode = dynamic_cast<WhileNode*>(node.get()))
        return InferenceReturnTypeBlock(whileNode->GetBodyNode(), locals);
 
    return builder.getInt32Ty();
}

void Compiler::DeclareConcat()
{
    llvm::Type* strTy = builder.getInt8Ty()->getPointerTo();
    std::vector<llvm::Type*> args = { strTy, strTy };

    llvm::FunctionType* funcType = llvm::FunctionType::get(strTy, args, false);
    concatFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "concat", module.get());
}

void Compiler::DeclareIntToStr()
{
    llvm::Type* strTy = builder.getInt8Ty()->getPointerTo();
    llvm::FunctionType* funcType = llvm::FunctionType::get(strTy, { builder.getInt32Ty() }, false);

    intToStrFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "int_to_string", module.get());
}

void Compiler::DeclareFree()
{
    llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), { builder.getInt8Ty()->getPointerTo() }, false);
    freeFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "free", module.get());
}

void Compiler::DeclarePrintf()
{
    llvm::FunctionType* funcType = llvm::FunctionType::get(builder.getInt32Ty(), { builder.getInt8Ty()->getPointerTo() }, true);
    printfFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "printf", module.get());
}

void Compiler::DeclareInputStrFunc()
{
    llvm::FunctionType* funcType = llvm::FunctionType::get(builder.getInt8Ty()->getPointerTo(), {}, false);
    inputStrFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "input_str", module.get());
}

void Compiler::DeclareInputNumFunc()
{
    llvm::FunctionType* funcType = llvm::FunctionType::get(builder.getInt32Ty(), {}, false);
    inputNumFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "input_num", module.get());
}

void Compiler::DeclareMalloc()
{
    llvm::FunctionType* funcType = llvm::FunctionType::get(builder.getInt8Ty()->getPointerTo(), { builder.getInt64Ty() }, false);
    mallocFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "malloc", module.get());
}

void Compiler::RegisterBuiltins()
{
    m_builtins["free"] = std::make_unique<BuiltinFree>(freeFunc);
    m_builtins["print"] = std::make_unique<BuiltinPrint>(printfFunc);
    m_builtins["println"] = std::make_unique<BuiltinPrintln>(printfFunc);
    m_builtins["input_str"] = std::make_unique<BuiltinInputStr>(inputStrFunc);
    m_builtins["input_num"] = std::make_unique<BuiltinInputNum>(inputNumFunc);
}

void Compiler::RegisterConstants()
{
    m_constants["true"] = llvm::ConstantInt::get(builder.getInt1Ty(), 1);
    m_constants["false"] = llvm::ConstantInt::get(builder.getInt1Ty(), 0);

    m_constants["null"] = llvm::ConstantPointerNull::get(llvm::PointerType::get(builder.getInt8Ty(), 0));

    m_constants["math_pi"] = llvm::ConstantFP::get(builder.getDoubleTy(), 3.141592653589793);
}

llvm::AllocaInst *Compiler::CreateEntryBlockAlloca(const std::string &name, llvm::Type *type)
{
    llvm::Function* func = builder.GetInsertBlock()->getParent();
    
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());

    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

llvm::Value *Compiler::IntToString(llvm::Value *val)
{
    llvm::Value* intToStrVal = builder.CreateCall(intToStrFunc, { val }, "intStrTmp");
    m_heapValues.insert(intToStrVal);
    return intToStrVal;
}

// Supported primitive names:
//   bool                        -> i1
//   uint8 / int8 / char         -> i8
//   uint16 / int16              -> i16
//   int / int32 / uint32        -> i32
//   int64 / uint64              -> i64
//   float                       -> f32
//   double                      -> f64
//   string                      -> ptr (i8*)
//   void                        -> void
//   <StructName>                -> ptr to the registered StructType
llvm::Type *Compiler::StringToLLVMType(const std::string &typeName)
{
    // 1-bit boolean
    if (typeName == "bool")
        return builder.getInt1Ty();
 
    // 8-bit integers
    if (typeName == "uint8" || typeName == "int8" || typeName == "char")
        return builder.getInt8Ty();
 
    // 16-bit integers
    if (typeName == "uint16" || typeName == "int16")
        return builder.getInt16Ty();
 
    // 32-bit integers
    if (typeName == "int" || typeName == "int32" || typeName == "uint32")
        return builder.getInt32Ty();
 
    // 64-bit integers
    if (typeName == "int64" || typeName == "uint64")
        return builder.getInt64Ty();
 
    // floating-point
    if (typeName == "float")
        return builder.getFloatTy();   // 32-bit IEEE float
 
    if (typeName == "double")
        return builder.getDoubleTy();
 
    // pointer-like / strings
    if (typeName == "string")
        return llvm::PointerType::get(builder.getInt8Ty(), 0);
 
    // void
    if (typeName == "void")
        return llvm::Type::getVoidTy(context);
 
    // named struct defined earlier. Passed by pointer when used as a parameter
    auto it = m_structs.types.find(typeName);
    if (it != m_structs.types.end())
        return llvm::PointerType::get(it->second, 0);
 
    std::cerr << "StringToLLVMType: unknow type '" << typeName << "', defaulting to i32\n";
    return builder.getInt32Ty();
}

std::optional<ArrayTypeInfo> Compiler::ParseArrayTypeName(const std::string &typeName)
{
    if (typeName.empty() || typeName.back() != ']')
        return std::nullopt;

    size_t bracketPos = typeName.find('[');
    if (bracketPos == std::string::npos)
        return std::nullopt;

    std::string elemTypeName = typeName.substr(0, bracketPos);
    std::string inside = typeName.substr(bracketPos + 1, typeName.size() - bracketPos - 2);

    ArrayTypeInfo info;
    info.elementTypeName = elemTypeName;
    info.elementType = StringToLLVMType(elemTypeName);

    if (inside == "dyn")
    {
        info.isDynamic = true;
        info.fixedSize = 0;
    }
    else
    {
        info.isDynamic = false;
        info.fixedSize = std::stoi(inside);
    }

    return info;
}

llvm::Value *Compiler::CreateFormatString(const std::string &fmt)
{
    builder.CreateGlobalStringPtr(fmt, "fmt");
    return nullptr;
}

llvm::Value *Compiler::GetArrayElementPtr(llvm::AllocaInst *alloca, llvm::Type *elementType, int length, bool isDynamic, llvm::Value *indexVal, const std::string &name)
{
    if (isDynamic)
    {
        llvm::Value* base = builder.CreateLoad(llvm::PointerType::get(elementType, 0), alloca, name);
        return builder.CreateGEP(elementType, base, indexVal, name + "ElemPtr");
    }

    llvm::Value* idx[] = { llvm::ConstantInt::get(builder.getInt32Ty(), 0), indexVal };
    return builder.CreateGEP(llvm::ArrayType::get(elementType, (uint64_t)length), alloca, idx, name + "ElemPtr");
}

std::string Compiler::DetectLinker()
{
#ifdef _WIN32
    if (std::system("where clang++ >nul 2>&1") == 0)
        return "clang++";
    if (std::system("where g++ >nul 2>&1") == 0)
        return "g++";
#else
    // Linux/macOS
    if (std::system("which clang++ > /dev/null 2>&1") == 0)
        return "clang++";
    if (std::system("which g++ > /dev/null 2>&1") == 0)
        return "g++";
#endif

    return "";
}
