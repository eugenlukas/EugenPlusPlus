#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

// x86-64 struct-by-value ABI classification (System V and Microsoft x64) and the
// coercion/decoercion helpers needed to pass/return structs correctly across a real C ABI
// boundary (extern function calls)
class AbiClassifier
{
public:
    enum class PassKind
    {
        Direct,     // struct fits in register(s)
        Indirect,   // struct goes through memory (byval on SysV, plain pointer-to-copy on Windows, or sret on return)
    };

    struct StructAbiInfo
    {
        PassKind kind = PassKind::Indirect;
        llvm::Type* coercedType = nullptr; // valid only when kind == Direct
        uint64_t size = 0;
    };

    // Per-extern-function ABI info: index-aligned with the function's logical (pre-ABI)
    // parameter list; nullopt = that parameter isn't a struct, no coercion needed.
    struct FunctionAbi
    {
        std::vector<std::optional<StructAbiInfo>> paramAbi;
        std::optional<StructAbiInfo> returnAbi;
    };

    AbiClassifier(llvm::LLVMContext& context, llvm::IRBuilder<>& builder, llvm::Module& module) : m_context(context), m_builder(builder), m_module(module) {}

    StructAbiInfo Classify(llvm::StructType* structTy);
    llvm::Value* CoerceForCall(llvm::Value* structPtr, const StructAbiInfo& abi);
    llvm::Value* CoerceScalarForParam(llvm::Value* val, llvm::Type* paramTy);
    llvm::Value* Decoerce(llvm::Value* coercedVal, llvm::StructType* structTy);

private:
    llvm::LLVMContext& m_context;
    llvm::IRBuilder<>& m_builder;
    llvm::Module& m_module;

private:
    llvm::AllocaInst* CreateEntryBlockAlloca(const std::string& name, llvm::Type* type);
};