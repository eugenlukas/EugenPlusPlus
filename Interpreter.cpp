#include "Interpreter.hpp"

RTResult Interpreter::Visit(std::shared_ptr<Node> node)
{
    if (auto number = dynamic_cast<NumberNode*>(node.get()))
        return Visit_NumberNode(*number);
    if (auto binOp = dynamic_cast<BinOpNode*>(node.get()))
        return Visit_BinOpNode(*binOp);
    if (auto unary = dynamic_cast<UnaryOpNode*>(node.get()))
        return Visit_UnaryOpNode(*unary);
    if (auto varAccess = dynamic_cast<VarAccessNode*>(node.get()))
        return Visit_VarAccsessNode(*varAccess);
    if (auto varAssign = dynamic_cast<VarAssignNode*>(node.get()))
        return Visit_VarAssignNode(*varAssign);

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

RTResult Interpreter::Visit_VarAccsessNode(VarAccessNode& node)
{
    RTResult res;

    std::variant<int, double, std::string> varName = node.GetVarNameToken().GetValue();
    std::optional<double> value = symbolTable.Get(std::get<std::string>(varName));

    if (!value.has_value())
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "'varName' is not defined"));

    return res.Success(value.value());
}

RTResult Interpreter::Visit_VarAssignNode(VarAssignNode& node)
{
    std::variant<int, double, std::string> varName = node.GetVarNameToken().GetValue();
    RTResult res_value = Visit(node.GetValueNode());

    if (res_value.HasError())
        return res_value;

    symbolTable.Set(std::get<std::string>(varName), res_value.GetValue().value());
    return res_value.Success(res_value.GetValue().value());
}

RTResult& RTResult::Success(double value)
{
    this->value = value;
    return *this;
}

RTResult& RTResult::Failure(std::unique_ptr<Error> error)
{
    this->error = std::move(error);
    return *this;
}

void SymbolTable::Set(const std::string& name, double value)
{
    symbols[name] = value;
}

std::optional<double> SymbolTable::Get(const std::string& name) const
{
    auto it = symbols.find(name);
    if (it != symbols.end())
    {
        return it->second;
    }
    // Check parent if not found locally
    if (parent != nullptr)
    {
        return parent->Get(name);
    }
    return std::nullopt;
}

bool SymbolTable::Remove(const std::string& name)
{
    return symbols.erase(name) > 0;
}
