#include "Compiler.hpp"

void Compiler::GenerateIR(std::shared_ptr<Node> rootNode)
{
    // Compile root node
    CompileNode(rootNode);

    module->print(llvm::outs(), nullptr);
}

llvm::Value *Compiler::CompileNode(std::shared_ptr<Node> node)
{
    if (auto n = dynamic_cast<NumberNode*>(node.get()))
        return Compile_NumberNode(n);

    if (auto n = dynamic_cast<StringNode*>(node.get()))
        return Compile_StringNode(n);

    if (auto n = dynamic_cast<BinOpNode*>(node.get()))
        return Compile_BinOpNode(n);

    if (auto n = dynamic_cast<ListNode*>(node.get()))
        return Compile_ListNode(n);

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

llvm::Value *Compiler::Compile_BinOpNode(BinOpNode *node)
{
    llvm::Value* left = CompileNode(node->GetLeftNode());
    llvm::Value* right = CompileNode(node->GetRightNode());

    std::string op = node->GetOpToken().GetType();

    if (op == TT_PLUS)
    {
        llvm::Type* leftTy = left->getType();
        llvm::Type* rightTy = right->getType();
        llvm::Type* strTy = builder.getInt8Ty()->getPointerTo();

        // double + double
        if ((leftTy->isDoubleTy() && rightTy->isDoubleTy()) || (leftTy->isIntegerTy() && rightTy->isIntegerTy()))
            return builder.CreateAdd(left, right, "addTmp");

        // string + string
        if (leftTy == strTy && rightTy == strTy)
        {
            return builder.CreateCall(concatFunc, { left, right }, "strcatTmp");
        }

        // string + int/double
        if (leftTy->isPointerTy() && (rightTy->isIntegerTy() || rightTy->isDoubleTy()))
        {
            llvm::Value* rightStr = IntToString(right);
            return builder.CreateCall(concatFunc, { left, rightStr}, "strcatTmp");
        }
    }
    if (op == TT_MINUS)
        return builder.CreateSub(left, right, "subTmp");
    if (op == TT_MUL)
        return builder.CreateMul(left, right, "mulTmp");
    if (op == TT_DIV)
        return builder.CreateSDiv(left, right, "divTmp"); //Signed
    if (op == TT_MOD)
        return builder.CreateSRem(left, right, "sremTmp"); //Signed
    if (op == TT_POW)
    {
        llvm::Function* powFunc = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::pow, {left->getType()});
        return builder.CreateCall(powFunc, {left, right}, "powTmp");
    }

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

    return builder.CreateLoad(alloca->getAllocatedType(), alloca, name);
}

llvm::Value *Compiler::Compile_VarAssignNode(VarAssignNode *node)
{
    std::string name = std::get<std::string>(node->GetVarNameToken().GetValue());
    llvm::Value* value = CompileNode(node->GetValueNode());
    llvm::Type* type = value->getType();

    llvm::AllocaInst* alloca;

    if (m_namedValues.find(name) == m_namedValues.end())
    {
        // first time -> allocate
        alloca = CreateEntryBlockAlloca(name, type);
        m_namedValues[name] = alloca;
    }
    else
        alloca = m_namedValues[name];

    if (alloca->getAllocatedType() != type)
    {
        std::cerr << "Type missmatch for variable: '" << name << "'\n";
        return nullptr;
    }

    builder.CreateStore(value, alloca);

    return value;
}

llvm::Value *Compiler::Compile_FuncDefNode(FuncDefNode *node)
{
    if (!node->GetVarNameTok().has_value())
    {
        std::cerr << "Anonymous functions not supported yet!\n";
        return nullptr;
    }

    std::string name = std::get<std::string>(node->GetVarNameTok()->GetValue());

    // Functiom type
    std::vector<llvm::Type*> argTypes(node->ArgNameToks().size(), builder.getInt32Ty());

    llvm::FunctionType* funcType = llvm::FunctionType::get(builder.getInt32Ty(), argTypes, false);

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
    m_namedValues.clear();

    // Store and allocate variables
    for (auto& arg : func->args())
    {
        llvm::AllocaInst* alloca = CreateEntryBlockAlloca(arg.getName().str(), arg.getType());
        builder.CreateStore(&arg, alloca);
        m_namedValues[arg.getName().str()] = alloca;
    }

    // Compile body
    llvm::Value* retVal = CompileNode(node->GetBodyNode());

    if (node->GetShouldAutoReturn())
        builder.CreateRet(retVal);
    else
        if (!block->getTerminator())
            builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0));

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

    // Builtin functions
    if (m_builtins.find(name) != m_builtins.end())
    {
        std::vector<llvm::Value*> args;

        for (auto& argNode : node->GetArgNodes())
            args.push_back(CompileNode(argNode));

        return m_builtins[name]->Codegen(builder, freeFunc, args);
    }

    // Normal functions
    if (m_functions.find(name) == m_functions.end())
    {
        std::cerr << "Unknown function: '" << name << "'!\n";
        return nullptr;
    }

    llvm::Function* func = m_functions[name];

    std::vector<llvm::Value*> args;
    for (auto& argNode : node->GetArgNodes())
        args.push_back(CompileNode(argNode));

    return builder.CreateCall(func, args, "calltmp");
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

void Compiler::RegisterBuiltins()
{
    m_builtins["free"] = std::make_unique<BuiltinFree>();
}

llvm::AllocaInst *Compiler::CreateEntryBlockAlloca(const std::string &name, llvm::Type *type)
{
    llvm::Function* func = builder.GetInsertBlock()->getParent();
    
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());

    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

llvm::Value *Compiler::IntToString(llvm::Value *val)
{
    return builder.CreateCall(intToStrFunc, { val }, "intStrTmp");
}
