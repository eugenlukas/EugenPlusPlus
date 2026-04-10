#pragma once
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <map>

#include <Nodes.hpp>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Value.h>

class Compiler
{
public:
    Compiler() : builder(context)
    {
        module = std::make_unique<llvm::Module>("main_module", context);
    }

    void GenerateIR(std::shared_ptr<Node> rootNode);

private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    std::map<std::string, llvm::AllocaInst*> m_namedValues;

private:
    llvm::Value* CompileNode(std::shared_ptr<Node> node);
    llvm::Value* Compile_NumberNode(NumberNode* node);
	//void Visit_StringNode(StringNode& node);
    llvm::Value* Compile_ListNode(ListNode* node);
    llvm::Value* Compile_BinOpNode(BinOpNode* node);
    llvm::Value* Compile_VarAccessNode(VarAccessNode* node);
    llvm::Value* Compile_VarAssignNode(VarAssignNode* node);
	//void Visit_UnaryOpNode(UnaryOpNode& node);
	//void Visit_IfNode(IfNode& node);
	//void Visit_ForNode(ForNode& node);
	//void Visit_WhileNode(WhileNode& node);
	//void Visit_FuncDefNode(FuncDefNode& node);
	//void Visit_CallNode(CallNode& node);
	//void Visit_ReturnNode(ReturnNode& node);
	//void Visit_ContinueNode(ContinueNode& node);
	//void Visit_BreakNode(BreakNode& node);
	//void Visit_ImportNode(ImportNode& node);

    llvm::AllocaInst* CreateEntryBlockAlloca(const std::string& name);
};