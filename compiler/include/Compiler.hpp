#pragma once
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <unordered_set>

#include <Nodes.hpp>
#include <BuiltinFunctions.hpp>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Value.h>
#include "llvm/IR/Intrinsics.h"
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <lld/Common/Driver.h>
#include <llvm/IR/LegacyPassManager.h> 
#include <llvm/Support/CodeGen.h> 
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"

struct VarInfo
{
    llvm::AllocaInst* alloca;
    bool isHeapAllocated;
};

struct LoopContext
{
    llvm::BasicBlock* continueBB;
    llvm::BasicBlock* breakBB;
};

class Compiler
{
public:
    Compiler() : builder(context)
    {
        module = std::make_unique<llvm::Module>("main_module", context);

        DeclareConcat();
        DeclareIntToStr();
        DeclareFree();
        DeclarePrintf();
        RegisterBuiltins();
    }

    void GenerateIR(std::shared_ptr<Node> rootNode, bool dumpIR);
    void EmitObjectFile(const std::string& filename);
    void LinkObjectFile(const std::string& filepath);

private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    std::vector<std::unordered_map<std::string, VarInfo>> m_scopes;
    std::unordered_set<llvm::Value*> m_heapValues;
    std::map<std::string, llvm::Function*> m_functions;
    std::vector<LoopContext> m_loopStack;
    std::map<std::string, std::unique_ptr<BuiltinFunction>> m_builtins;

    llvm::Function* concatFunc;
    llvm::Function* intToStrFunc;
    llvm::Function* freeFunc;
    llvm::Function* printfFunc;

private:
    llvm::Value* CompileNode(std::shared_ptr<Node> node);
    llvm::Value* Compile_NumberNode(NumberNode* node);
    llvm::Value* Compile_StringNode(StringNode* node);
    llvm::Value* Compile_ListNode(ListNode* node);
    llvm::Value* Compile_BinOpNode(BinOpNode* node);
    llvm::Value* Compile_VarAccessNode(VarAccessNode* node);
    llvm::Value* Compile_VarAssignNode(VarAssignNode* node);
    llvm::Value* Compile_UnaryOpNode(UnaryOpNode* node);
    llvm::Value* Compile_IfNode(IfNode* node, llvm::BasicBlock* existingMergeBB = nullptr);
    llvm::Value* Compile_ForNode(ForNode* node);
	//void Visit_WhileNode(WhileNode& node);
    llvm::Value* Compile_FuncDefNode(FuncDefNode* node);
    llvm::Value* Compile_CallNode(CallNode* node);
    llvm::Value* Compile_ReturnNode(ReturnNode* node);
    llvm::Value* Compile_ContinueNode(ContinueNode* node);
    llvm::Value* Compile_BreakNode(BreakNode* node);
	//void Visit_ImportNode(ImportNode& node);

    void PushScope();
    void PopScope();

    VarInfo* FindVariable(const std::string& name);
    void SetVariable(const std::string& name, VarInfo info);

    void FreeLocalHeapValues();

    void DeclareConcat();
    void DeclareIntToStr();
    void DeclareFree();
    void DeclarePrintf();
    void RegisterBuiltins();

    llvm::AllocaInst* CreateEntryBlockAlloca(const std::string& name, llvm::Type* type);
    llvm::Value* IntToString(llvm::Value* val);
    llvm::Value* CreateFormatString(const std::string& fmt);

    std::string DetectLinker();
};