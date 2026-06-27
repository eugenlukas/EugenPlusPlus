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
//#include <lld/Common/Driver.h>
#include <llvm/IR/LegacyPassManager.h> 
#include <llvm/Support/CodeGen.h> 
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"

struct VarInfo
{
    llvm::AllocaInst* alloca;
    bool isHeapAllocated;
};

// Bundles everything the compiler needs to know about struct definitions
struct StructRegistry
{
    // struct name - llvm named struct type
    std::unordered_map<std::string, llvm::StructType*> types;
    // struct name - ordered attribute list
    std::unordered_map<std::string, std::vector<StructAttributeToken>> fields;
    // Variable name - struct name
    std::unordered_map<std::string, std::string> varToType;

    bool HasType(const std::string& name) const { return types.count(name) > 0; }
    bool IsStructVar(const std::string& varName) const { return varToType.count(varName) > 0; }
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

    void SetMainFilepath(const std::string& path) { m_mainFilepath = path; }

    void GenerateIR(std::shared_ptr<Node> rootNode, bool dumpIR);
    void EmitObjectFile(const std::string& filename);
    void LinkObjectFile(const std::string& filepath);

private:
    std::string m_mainFilepath; 

    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;

    std::vector<std::unordered_map<std::string, VarInfo>> m_scopes;
    std::unordered_set<llvm::Value*> m_heapValues;
    std::map<std::string, llvm::Function*> m_functions;
    std::unordered_map<std::string, std::unordered_map<std::string, llvm::Function*>> m_moduleFunctions;
    std::unordered_map<std::string, std::unordered_map<std::string, VarInfo>> m_moduleVariables; // global variables / top-level
    std::vector<LoopContext> m_loopStack;
    std::map<std::string, std::unique_ptr<BuiltinFunction>> m_builtins;
    std::unordered_map<std::string, std::string>  m_linkedLibs;
    std::unordered_map<std::string, std::unordered_map<std::string, llvm::Function*>> m_externFunctions;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_externReturnTypeStrings;
    StructRegistry m_structs;

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
    llvm::Value* Compile_WhileNode(WhileNode* node);
    llvm::Value* Compile_FuncDefNode(FuncDefNode* node);
    llvm::Value* Compile_CallNode(CallNode* node);
    llvm::Value* Compile_ReturnNode(ReturnNode* node);
    llvm::Value* Compile_ContinueNode(ContinueNode* node);
    llvm::Value* Compile_BreakNode(BreakNode* node);
    llvm::Value* Compile_ModuleNode(ModuleNode* node);
    llvm::Value* Compile_LinkNode(LinkNode* node);
    llvm::Value* Compile_ExternNode(ExternNode* node);
    llvm::Value* Compile_StructDefNode(StructDefNode* node);

    void PushScope();
    void PopScope();

    VarInfo* FindVariable(const std::string& name);
    void SetVariable(const std::string& name, VarInfo info);
    // flattens m_scopes into a single name→VarInfo map so we can snapshot all currently visible variables before compiling a module and diff afterwards
    std::unordered_map<std::string, VarInfo> CollectAllVariables() const;
    void FreeLocalHeapValues();
    // recursively scans a function body for any "paramName::field" access or assignment. Returns the struct type name whose fields match, or "" if the parameter is not used as a struct inside this body.
    std::string ScanForStructParamUsage(const std::string& paramName, std::shared_ptr<Node> body);

    using LocalTypeMap = std::unordered_map<std::string, llvm::Type*>;
    llvm::Type* InferenceExprType(std::shared_ptr<Node> node, const LocalTypeMap& locals);
    llvm::Type* InferenceReturnType(std::shared_ptr<Node> body, bool autoReturn);
    llvm::Type* InferenceReturnTypeBlock(std::shared_ptr<Node> node, LocalTypeMap& locals);

    void DeclareConcat();
    void DeclareIntToStr();
    void DeclareFree();
    void DeclarePrintf();
    void RegisterBuiltins();

    llvm::AllocaInst* CreateEntryBlockAlloca(const std::string& name, llvm::Type* type);
    llvm::Value* IntToString(llvm::Value* val);
    llvm::Type* StringToLLVMType(const std::string& typeName);
    llvm::Value* CreateFormatString(const std::string& fmt);

    std::string DetectLinker();
};