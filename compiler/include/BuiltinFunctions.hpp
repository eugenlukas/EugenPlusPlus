#pragma once

#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <iostream>

class BuiltinFunction
{
public:
    ~BuiltinFunction() = default;
    BuiltinFunction(llvm::Function* func)
    {
        if (func == nullptr)
            std::cerr << "func not declared when initializing builtin function!\n";
            
        this->func = func; 
    }

    virtual llvm::Value* Codegen(llvm::IRBuilder<>& builder, std::vector<llvm::Value*> args) = 0;
    virtual std::string GetName() const = 0;

protected:
    llvm::Function* func;
};

class BuiltinPrint : public BuiltinFunction
{
public:
    using BuiltinFunction::BuiltinFunction;
    llvm::Value* Codegen(llvm::IRBuilder<>& builder, std::vector<llvm::Value*> args) override
    {
        if (args.size() != 1)
        {
            std::cerr << "print expects 1 argument";
            return nullptr;
        }

        llvm::Value* arg = args[0];
        llvm::LLVMContext& context = builder.getContext();

        llvm::Value* formatStr;

        // string
        if (arg->getType() == builder.getInt8Ty()->getPointerTo())
        {
            formatStr = builder.CreateGlobalStringPtr("%s", "fmt");
            return builder.CreateCall(func, { formatStr, arg });
        }

        // int
        if (arg->getType()->isIntegerTy())
        {
            auto* intTy = llvm::cast<llvm::IntegerType>(arg->getType());

            if (intTy->getBitWidth() < 32)
                arg = builder.CreateZExt(arg, builder.getInt32Ty(), "promote");

            formatStr = builder.CreateGlobalStringPtr("%d", "fmt");
            return builder.CreateCall(func, { formatStr, arg });
        }

        std::cerr << "Unsupported type in print!\n";
        return nullptr;
    }

    std::string GetName() const override { return "<built-in function 'print'>"; }
};

class BuiltinPrintln : public BuiltinFunction
{
public:
    using BuiltinFunction::BuiltinFunction;
    llvm::Value* Codegen(llvm::IRBuilder<>& builder, std::vector<llvm::Value*> args) override
    {
        if (args.size() != 1)
        {
            std::cerr << "print expects 1 argument";
            return nullptr;
        }

        llvm::Value* arg = args[0];
        llvm::LLVMContext& context = builder.getContext();

        llvm::Value* formatStr;

        // string
        if (arg->getType() == builder.getInt8Ty()->getPointerTo())
        {
            formatStr = builder.CreateGlobalStringPtr("%s\n", "fmt");
            return builder.CreateCall(func, { formatStr, arg });
        }

        // int
        if (arg->getType()->isIntegerTy())
        {
            auto* intTy = llvm::cast<llvm::IntegerType>(arg->getType());

            if (intTy->getBitWidth() < 32)
                arg = builder.CreateZExt(arg, builder.getInt32Ty(), "promote");

            formatStr = builder.CreateGlobalStringPtr("%d\n", "fmt");
            return builder.CreateCall(func, { formatStr, arg });
        }

        std::cerr << "Unsupported type in println!\n";
        return nullptr;
    }

    std::string GetName() const override { return "<built-in function 'println'>"; }
};

class BuiltinFree : public BuiltinFunction
{
public:
    using BuiltinFunction::BuiltinFunction;
    llvm::Value* Codegen(llvm::IRBuilder<>& builder, std::vector<llvm::Value*> args) override
    {
        if (args.size() != 1)
        {
            std::cerr << "free expects 1 argument!\n";
            return nullptr;
        }

        llvm::Value* arg = args[0];

        if (arg->getType() != builder.getInt8Ty()->getPointerTo())
        {
            std::cerr << "free expects a string (i8*)!\n";
            return nullptr;
        }

        return builder.CreateCall(func, { arg });
    }

    std::string GetName() const override { return "<built-in function 'free'>"; }
};