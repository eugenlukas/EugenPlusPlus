#pragma once

#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <iostream>

class BuiltinFunction
{
public:
    ~BuiltinFunction() = default;

    virtual llvm::Value* Codegen(llvm::IRBuilder<>& builder, llvm::Function* func, std::vector<llvm::Value*> args) = 0;
    virtual std::string GetName() const = 0;
};

class BuiltinFree : public BuiltinFunction
{
public:
    llvm::Value* Codegen(llvm::IRBuilder<>& builder, llvm::Function* func, std::vector<llvm::Value*> args) override
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