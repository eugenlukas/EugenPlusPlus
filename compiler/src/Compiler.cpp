#include "Compiler.hpp"
#include <Helper.hpp>

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
        "-lruntime " +
        "-o \"" + exeName + "\"";

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
    {
        std::cerr << "ToDo: Implement 'unary operation' node\n";
        return nullptr;
    }

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

    if (auto n = dynamic_cast<IfNode*>(node.get()))
        return Compile_IfNode(n);

    if (auto n = dynamic_cast<ForNode*>(node.get()))
        return Compile_ForNode(n);

    if (auto n = dynamic_cast<WhileNode*>(node.get()))
    {
        std::cerr << "ToDo: Implement 'while' node\n";
        return nullptr;
    }

    if (auto n = dynamic_cast<ContinueNode*>(node.get()))
        return Compile_ContinueNode(n);

    if (auto n = dynamic_cast<BreakNode*>(node.get()))
        return Compile_BreakNode(n);

    if (auto n = dynamic_cast<ModuleNode*>(node.get()))
    {
        std::cerr << "ToDo: Implement 'module' node\n";
        return nullptr;
    }

    if (auto n = dynamic_cast<LinkNode*>(node.get()))
    {
        std::cerr << "ToDo: Implement 'link' node\n";
        return nullptr;
    }

    if (auto n = dynamic_cast<ExternNode*>(node.get()))
    {
        std::cerr << "ToDo: Implement 'extern' node\n";
        return nullptr;
    }

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
            return builder.CreateICmpEQ(left, right, "eqtmp");
        if (op == TT_NEQ)
            return builder.CreateICmpNE(left, right, "netmp");
        if (op == TT_LT)
            return builder.CreateICmpSLT(left, right, "lttmp"); // Signed
        if (op == TT_GT)
            return builder.CreateICmpSGT(left, right, "gttmp"); // Signed
        if (op == TT_LTEQ)
            return builder.CreateICmpSLE(left, right, "ltetmp"); // Signed
        if (op == TT_GTEQ)
            return builder.CreateICmpSGE(left, right, "gtetmp"); // Signed
        if (node->GetOpToken().Matches(TT_KEYWORD, "AND"))
            return builder.CreateAnd(left, right, "andtmp");
        if (node->GetOpToken().Matches(TT_KEYWORD, "OR"))
            return builder.CreateOr(left, right, "ortmp");

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

    VarInfo* var = FindVariable(name);
    if (!var)
    {
        std::cerr << "Undefined variable: " << name << "\n";
        return nullptr;
    }

    return builder.CreateLoad(var->alloca->getAllocatedType(), var->alloca, name);
}

llvm::Value *Compiler::Compile_VarAssignNode(VarAssignNode *node)
{
    std::string name = std::get<std::string>(node->GetVarNameToken().GetValue());
    llvm::Value* value = CompileNode(node->GetValueNode());
    llvm::Type* type = value->getType();

    llvm::AllocaInst* alloca = nullptr;
    bool isHeap = m_heapValues.contains(value);

    VarInfo* existing = FindVariable(name);
    if (!existing)
    {
        // first time -> allocate
        alloca = CreateEntryBlockAlloca(name, type);
        SetVariable(name, { alloca, isHeap });
        builder.CreateStore(value, alloca);
    }
    else
    {
        builder.CreateStore(value, existing->alloca);
    }

    if (alloca->getAllocatedType() != type)
    {
        std::cerr << "Type missmatch for variable: '" << name << "'\n";
        return nullptr;
    }

    builder.CreateStore(value, alloca);

    return value;
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

    // get step for consition
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
    PushScope();

    // Store and allocate variables
    for (auto& arg : func->args())
    {
        llvm::AllocaInst* alloca = CreateEntryBlockAlloca(arg.getName().str(), arg.getType());
        builder.CreateStore(&arg, alloca);
        SetVariable(arg.getName().str(), { alloca, false });
    }

    // Compile body
    llvm::Value* retVal = CompileNode(node->GetBodyNode());

    // Free all localy created heap values at end of function
    FreeLocalHeapValues();

    PopScope();

    if (node->GetShouldAutoReturn())
        builder.CreateRet(retVal);
    else
    {
        llvm::BasicBlock* currentBB = builder.GetInsertBlock();
        if (!currentBB->getTerminator())
            builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0));
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

    // Builtin functions
    if (m_builtins.find(name) != m_builtins.end())
    {
        std::vector<llvm::Value*> args;

        for (auto& argNode : node->GetArgNodes())
            args.push_back(CompileNode(argNode));

        return m_builtins[name]->Codegen(builder, args);
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

void Compiler::RegisterBuiltins()
{
    m_builtins["free"] = std::make_unique<BuiltinFree>(freeFunc);
    m_builtins["print"] = std::make_unique<BuiltinPrint>(printfFunc);
    m_builtins["println"] = std::make_unique<BuiltinPrintln>(printfFunc);
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
    return val;
}

llvm::Value *Compiler::CreateFormatString(const std::string &fmt)
{
    builder.CreateGlobalStringPtr(fmt, "fmt");
    return nullptr;
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
