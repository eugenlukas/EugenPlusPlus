#include "Compiler.hpp"

void Compiler::GenerateIR(std::shared_ptr<Node> rootNode)
{
    // Create main function
    llvm::FunctionType* funcType = llvm::FunctionType::get(builder.getInt32Ty(), false);

    llvm::Function* mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module.get());

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", mainFunc);

    builder.SetInsertPoint(entry);

    // Compile root node
    CompileNode(rootNode);

    // default return 0
    builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0));

    llvm::verifyFunction(*mainFunc);

    module->print(llvm::outs(), nullptr);
}

llvm::Value *Compiler::CompileNode(std::shared_ptr<Node> node)
{
    if (auto n = dynamic_cast<NumberNode*>(node.get()))
        return Compile_NumberNode(n);

    if (auto n = dynamic_cast<BinOpNode*>(node.get()))
        return Compile_BinOpNode(n);

    if (auto n = dynamic_cast<ListNode*>(node.get()))
        return Compile_ListNode(n);

    if (auto n = dynamic_cast<VarAccessNode*>(node.get()))
        return Compile_VarAccessNode(n);

    if (auto n = dynamic_cast<VarAssignNode*>(node.get()))
        return Compile_VarAssignNode(n);

    std::cerr << "Unknown node type\n";
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

llvm::Value *Compiler::Compile_ListNode(ListNode *node)
{
    llvm::Value* last = nullptr;

    for (auto& stmt : node->GetElementNodes())
        last = CompileNode(stmt);

    return last;
}

llvm::Value *Compiler::Compile_BinOpNode(BinOpNode *node)
{
    llvm::Value* left = CompileNode(node->GetLeftNode());
    llvm::Value* right = CompileNode(node->GetRightNode());

    std::string op = node->GetOpToken().GetType();

    if (op == TT_PLUS)
        return builder.CreateAdd(left, right, "addTmp");
    if (op == TT_MINUS)
        return builder.CreateSub(left, right, "subTmp");
    if (op == TT_MUL)
        return builder.CreateMul(left, right, "mulTmp");
    if (op == TT_DIV)
        return builder.CreateSDiv(left, right, "divTmp");

    std::cerr << "Unknown binary operation\n";
    return nullptr;
}

llvm::Value *Compiler::Compile_VarAccessNode(VarAccessNode *node)
{
    std::string name = std::get<std::string>(node->GetVarNameToken().GetValue());

    if (m_namedValues.find(name) == m_namedValues.end())
    {
        std::cerr << "Undefined variable: " << name << "\n";
        return nullptr;
    }

    llvm::AllocaInst* alloca = m_namedValues[name];

    return builder.CreateLoad(builder.getInt32Ty(), alloca, name);
}

llvm::Value *Compiler::Compile_VarAssignNode(VarAssignNode *node)
{
    std::string name = std::get<std::string>(node->GetVarNameToken().GetValue());
    llvm::Value* value = CompileNode(node->GetValueNode());

    llvm::AllocaInst* alloca;

    if (m_namedValues.find(name) == m_namedValues.end())
    {
        // first time -> allocate
        alloca = CreateEntryBlockAlloca(name);
        m_namedValues[name] = alloca;
    }
    else
        alloca = m_namedValues[name];

    builder.CreateStore(value, alloca);

    return value;
}

llvm::AllocaInst *Compiler::CreateEntryBlockAlloca(const std::string &name)
{
    llvm::Function* func = builder.GetInsertBlock()->getParent();
    
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());

    return tmpBuilder.CreateAlloca(builder.getInt32Ty(), nullptr, name);
}
