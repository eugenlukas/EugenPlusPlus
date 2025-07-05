#include "Interpreter.hpp"
#include <functional>

RTResult Interpreter::Visit(std::shared_ptr<Node> node)
{
    if (auto number = dynamic_cast<NumberNode*>(node.get()))
        return Visit_NumberNode(*number);
    if (auto binOp = dynamic_cast<BinOpNode*>(node.get()))
        return Visit_BinOpNode(*binOp);
    if (auto unary = dynamic_cast<UnaryOpNode*>(node.get()))
        return Visit_UnaryOpNode(*unary);
    if (auto varAccess = dynamic_cast<VarAccessNode*>(node.get()))
        return Visit_VarAccessNode(*varAccess);
    if (auto varAssign = dynamic_cast<VarAssignNode*>(node.get()))
        return Visit_VarAssignNode(*varAssign);
    if (auto varIf = dynamic_cast<IfNode*>(node.get()))
        return Visit_IfNode(*varIf);
    if (auto for_ = dynamic_cast<ForNode*>(node.get()))
        return Visit_ForNode(*for_);
    if (auto while_ = dynamic_cast<WhileNode*>(node.get()))
        return Visit_WhileNode(*while_);
    if (auto funcDef = dynamic_cast<FuncDefNode*>(node.get()))
        return Visit_FuncDefNode(*funcDef);
    if (auto call = dynamic_cast<CallNode*>(node.get()))
        return Visit_CallNode(*call);

    return RTResult().Failure(std::make_unique<RuntimeError>(Position(), Position(), "Unknown node type"));
}

RTResult Interpreter::Visit_NumberNode(NumberNode& node)
{
    auto val = node.GetToken().GetValue();
    double num = 0;

    if (std::holds_alternative<int>(val))
        num = static_cast<double>(std::get<int>(val));
    else if (std::holds_alternative<double>(val))
        num = std::get<double>(val);

    return RTResult().Success(num);
}

RTResult Interpreter::Visit_BinOpNode(BinOpNode& node)
{
    auto left = Visit(node.GetLeftNode());
    auto right = Visit(node.GetRightNode());

    if (left.HasError())
        return left;
    if (right.HasError())
        return right;

    double l = left.GetValue().value();
    double r = right.GetValue().value();

    Position pos_start = node.GetOpToken().GetPosStart();
    Position pos_end = node.GetOpToken().GetPosEnd();

    if (node.GetOpToken().GetType() == TT_PLUS)
        return RTResult().Success(l + r);
    else if (node.GetOpToken().GetType() == TT_MINUS)
        return RTResult().Success(l - r);
    else if (node.GetOpToken().GetType() == TT_MUL)
        return RTResult().Success(l * r);
    else if (node.GetOpToken().GetType() == TT_DIV)
    {
        if (r == 0)
            return RTResult().Failure(std::make_unique<RuntimeError>(pos_start, pos_end, "Division by zero"));
        return RTResult().Success(l / r);
    }
    else if (node.GetOpToken().GetType() == TT_POW)
        return RTResult().Success(pow(l, r));
    else if (node.GetOpToken().GetType() == TT_EQEQ)
        return RTResult().Success(l == r);
    else if (node.GetOpToken().GetType() == TT_NEQ)
        return RTResult().Success(l != r);
    else if (node.GetOpToken().GetType() == TT_LT)
        return RTResult().Success(l < r);
    else if (node.GetOpToken().GetType() == TT_GT)
        return RTResult().Success(l > r);
    else if (node.GetOpToken().GetType() == TT_LTEQ)
        return RTResult().Success(l <= r);
    else if (node.GetOpToken().GetType() == TT_GTEQ)
        return RTResult().Success(l >= r);
    else if (node.GetOpToken().Matches(TT_KEYWORD, "AND"))
        return RTResult().Success(l && r);
    else if (node.GetOpToken().Matches(TT_KEYWORD, "OR"))
        return RTResult().Success(l || r);

    return RTResult().Failure(std::make_unique<RuntimeError>(pos_start, pos_end, "Unknown binary operator"));
}

RTResult Interpreter::Visit_UnaryOpNode(UnaryOpNode& node)
{
    RTResult res_num = Visit(node.GetNode());
    if (res_num.HasError()) return res_num;

    double num = res_num.GetValue().value();
    std::string op_type = node.GetOpToken().GetType();

    if (op_type == TT_MINUS)
        return RTResult().Success(-num);
    if (op_type == TT_PLUS)
        return RTResult().Success(+num);
    if (node.GetOpToken().Matches(TT_KEYWORD, "NOT"))
        return RTResult().Success(num == 0 ? 1 : 0);

    return RTResult().Failure(
        std::make_unique<RuntimeError>(
            node.GetOpToken().GetPosStart(),
            node.GetOpToken().GetPosEnd(),
            "Unknown unary operator"
        )
    );
}

RTResult Interpreter::Visit_VarAccessNode(VarAccessNode& node)
{
    RTResult res;

    std::string varName = std::get<std::string>(node.GetVarNameToken().GetValue());
    auto value = symbolTable.Get(varName);

    if (!value.has_value())
    {
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "'" + varName + "' is not defined"));
    }

    if (std::holds_alternative<double>(value.value()))
    {
        return res.Success(std::get<double>(value.value()));
    }
    else
    {
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Variable is not a number"));
    }
}

RTResult Interpreter::Visit_VarAssignNode(VarAssignNode& node)
{
    std::string varName = std::get<std::string>(node.GetVarNameToken().GetValue());
    RTResult res_value = Visit(node.GetValueNode());

    if (res_value.HasError())
        return res_value;

    symbolTable.Set(varName, res_value.GetValue().value());
    return res_value.Success(res_value.GetValue().value());
}

RTResult Interpreter::Visit_IfNode(IfNode& node)
{
    RTResult res;

    for (IfCase& ifCase : node.GetCases())
    {
        RTResult conditionValue = Visit(ifCase.GetCondition());
        if (conditionValue.HasError())
            return conditionValue;

        if (conditionValue.GetValue().value() != 0)
        {
            RTResult exprValue = Visit(ifCase.GetExpr());
            if (exprValue.HasError())
                return exprValue;

            return res.Success(exprValue.GetValue().value());
        }
    }

    if (node.GetElseCase() != nullptr)
    {
        RTResult elseValue = Visit(node.GetElseCase());
        if (elseValue.HasError())
            return elseValue;

        return res.Success(elseValue.GetValue().value());
    }

    return res.Success(std::nullopt);
}

RTResult Interpreter::Visit_ForNode(ForNode& node)
{
    RTResult res;

    RTResult startValue = Visit(node.GetStartValueNode());
    if (startValue.HasError())
        return startValue;

    RTResult endValue = Visit(node.GetEndValueNode());
    if (endValue.HasError())
        return endValue;

    RTResult stepValue;
    if (node.GetStepValueNode() != nullptr)
    {
        stepValue = Visit(node.GetStepValueNode());
        if (stepValue.HasError())
            return startValue;
    }
    else
        stepValue.SetValue(1);

    std::function<bool(int)> condition;

    if (stepValue.GetValue().value() >= 0)
        condition = [=](int i) { return i < endValue.GetValue().value(); };
    else
        condition = [=](int i) { return i > endValue.GetValue().value(); };

    for (int i = startValue.GetValue().value(); condition(i); i += stepValue.GetValue().value())
    {
        symbolTable.Set(std::get<std::string>(node.GetVarNameTok().GetValue()), static_cast<double>(i));

        res = Visit(node.GetBodyNode());
        if (res.HasError())
            return res;
    }

    return res.Success(std::nullopt);
}

RTResult Interpreter::Visit_WhileNode(WhileNode& node)
{
    RTResult res;

    while (true)
    {
        auto condition = Visit(node.GetConditionNode());
        if (condition.HasError())
            return condition;

        if (condition.GetValue() == 0)
            break;

        res = Visit(node.GetBodyNode());
        if (res.HasError())
            return res;
    }

    return res.Success(std::nullopt);
}

RTResult Interpreter::Visit_FuncDefNode(FuncDefNode& node)
{
    RTResult res;

    if (node.GetVarNameTok().has_value())
    {
        std::string funcName = std::get<std::string>(node.GetVarNameTok().value().GetValue());
        symbolTable.Set(funcName, node);
    }

    return res.Success(std::nullopt);
}

RTResult Interpreter::Visit_CallNode(CallNode& node)
{
    RTResult res;
    std::string funcName;
    if (auto varAccess = dynamic_cast<VarAccessNode*>(node.GetNodeToCall().get()))
        funcName = std::get<std::string>(varAccess->GetVarNameToken().GetValue());
    else
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Invalid function name"));

    auto funcValue = symbolTable.Get(funcName);
    if (!funcValue.has_value() || !std::holds_alternative<FuncDefNode>(funcValue.value()))
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Function '" + funcName + "' not found"));

    FuncDefNode funcNode = std::get<FuncDefNode>(funcValue.value());

    if (node.GetArgNodes().size() != funcNode.GetArgNameToks().size())
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Incorrect number of arguments"));

    // Save current symbol table
    SymbolTable localSymbolTable(&symbolTable);

    // Assign arguments
    for (size_t i = 0; i < node.GetArgNodes().size(); ++i)
    {
        auto argRes = Visit(node.GetArgNodes()[i]);
        if (argRes.HasError()) return argRes;

        std::string argName = std::get<std::string>(funcNode.GetArgNameToks()[i].GetValue());
        localSymbolTable.Set(argName, argRes.GetValue().value());
    }

    // Execute function body
    Interpreter funcInterpreter(localSymbolTable);
    auto result = funcInterpreter.Visit(funcNode.GetBodyNode());
    if (result.HasError()) return result;

    return res.Success(result.GetValue());
}

RTResult& RTResult::Success(std::optional<double> value)
{
    this->value = value;
    return *this;
}

RTResult& RTResult::Failure(std::unique_ptr<Error> error)
{
    this->error = std::move(error);
    return *this;
}
