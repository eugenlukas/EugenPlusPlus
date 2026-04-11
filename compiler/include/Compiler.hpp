#pragma once
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <map>

#include <Nodes.hpp>
#include <BuiltinFunctions.hpp>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Value.h>
#include "llvm/IR/Intrinsics.h"

class Compiler
{
public:
    Compiler() : builder(context)
    {
        module = std::make_unique<llvm::Module>("main_module", context);

        DeclareConcat();
        DeclareIntToStr();
        DeclareFree();
        RegisterBuiltins();
    }

    void GenerateIR(std::shared_ptr<Node> rootNode);

private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    std::map<std::string, llvm::AllocaInst*> m_namedValues;
    std::map<std::string, llvm::Function*> m_functions;
    std::map<std::string, std::unique_ptr<BuiltinFunction>> m_builtins;

    llvm::Function* concatFunc;
    llvm::Function* intToStrFunc;
    llvm::Function* freeFunc;

private:
    llvm::Value* CompileNode(std::shared_ptr<Node> node);
    llvm::Value* Compile_NumberNode(NumberNode* node);
    llvm::Value* Compile_StringNode(StringNode* node);
    llvm::Value* Compile_ListNode(ListNode* node);
    llvm::Value* Compile_BinOpNode(BinOpNode* node);
    llvm::Value* Compile_VarAccessNode(VarAccessNode* node);
    llvm::Value* Compile_VarAssignNode(VarAssignNode* node);
	//void Visit_UnaryOpNode(UnaryOpNode& node);
	//void Visit_IfNode(IfNode& node);
	//void Visit_ForNode(ForNode& node);
	//void Visit_WhileNode(WhileNode& node);
    llvm::Value* Compile_FuncDefNode(FuncDefNode* node);
    llvm::Value* Compile_CallNode(CallNode* node);
    llvm::Value* Compile_ReturnNode(ReturnNode* node);
	//void Visit_ContinueNode(ContinueNode& node);
	//void Visit_BreakNode(BreakNode& node);
	//void Visit_ImportNode(ImportNode& node);

    void DeclareConcat();
    void DeclareIntToStr();
    void DeclareFree();
    void RegisterBuiltins();

    llvm::AllocaInst* CreateEntryBlockAlloca(const std::string& name, llvm::Type* type);
    llvm::Value* IntToString(llvm::Value* val);
};