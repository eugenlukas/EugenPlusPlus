#include "Interpreter.hpp"

RTResult Interpreter::Visit(std::shared_ptr<Node> node)
{
    if (auto number = dynamic_cast<NumberNode*>(node.get()))
        return Visit_NumberNode(*number);
    if (auto binOp = dynamic_cast<BinOpNode*>(node.get()))
        return Visit_BinOpNode(*binOp);
    if (auto unary = dynamic_cast<UnaryOpNode*>(node.get()))
        return Visit_UnaryOpNode(*unary);

    return RTResult().Failure(std::make_unique<RuntimeError>(Position(), Position(), "Unknown node type"));
}

RTResult Interpreter::Visit_NumberNode(NumberNode& node)
{
    auto val = node.GetToken().GetValue();
    double num = 0;

    if (std::holds_alternative<int>(val))
        num = static_cast<double>(std::get<int>(val));
    else
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

    return RTResult().Failure(
        std::make_unique<RuntimeError>(
            node.GetOpToken().GetPosStart(),
            node.GetOpToken().GetPosEnd(),
            "Unknown unary operator"
        )
    );
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
