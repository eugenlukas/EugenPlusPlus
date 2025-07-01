#include "Parser.hpp"
#include <algorithm>

Parser::Parser(std::vector<Token> tokens)
{
	this->tokens = tokens;
	Advance();
}

Token Parser::Advance()
{
	tokIdx++;
	if (tokIdx < tokens.size())
		currentToken = tokens[tokIdx];

	return currentToken;
}

ParseResult Parser::Parse()
{
	//std::cout << "Parse!" << std::endl;
	ParseResult res = Expr();
	if (!res.HasError() && currentToken.GetType() != TT_EOF)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '+', '-', '*' or '/'"));
	return res;
}

ParseResult Parser::Atom()
{
	ParseResult res = ParseResult();
	Token tok = currentToken;

	if (tok.GetType() == TT_INT || tok.GetType() == TT_FLOAT)
	{
		res.RegisterAdvancement(Advance());
		return res.Success(std::make_shared<NumberNode>(tok));
	}
	else if (tok.GetType() == TT_IDENTIFIER)
	{
		res.RegisterAdvancement(Advance());
		return res.Success(std::make_shared<VarAccessNode>(tok));
	}
	else if (tok.GetType() == TT_LPAREN)
	{
		res.RegisterAdvancement(Advance());
		std::shared_ptr<Node> expr = res.Register(Expr());
		if (res.HasError())
			return res;
		if (currentToken.GetType() == TT_RPAREN)
		{
			res.RegisterAdvancement(Advance());
			return res.Success(expr);
		}
		else
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ')'"));
	}

	return res.Failure(std::make_unique<InvalidSyntaxError>(tok.GetPosStart(), tok.GetPosEnd(), "Expected int, float, identifier, '+', '-' or ''("));
}

ParseResult Parser::Power()
{
	std::vector<std::string> ops = { TT_POW};
	return BinOp([this]() {return Atom(); }, ops, [this]() {return Factor(); });
}

ParseResult Parser::Factor()
{
	ParseResult res = ParseResult();
	Token tok = currentToken;

	if (tok.GetType() == TT_PLUS || tok.GetType() == TT_MINUS)
	{
		res.RegisterAdvancement(Advance());
		std::shared_ptr<Node> factor = res.Register(Factor());
		if (res.HasError())
			return res;
		return res.Success(std::make_shared<UnaryOpNode>(tok, factor));
	}

	return Power();
}

ParseResult Parser::Term()
{
	std::vector<std::string> ops = { TT_MUL, TT_DIV };
	return BinOp([this]() {return Factor(); }, ops);
}

ParseResult Parser::ArithExpr()
{
	std::vector<std::string> ops = { TT_PLUS, TT_MINUS };
	return BinOp([this]() {return Term(); }, ops);
}

ParseResult Parser::CompExpr()
{
	ParseResult res;

	if (currentToken.Matches(TT_KEYWORD, "NOT"))
	{
		Token opToken = currentToken;
		res.RegisterAdvancement(Advance());

		std::shared_ptr<Node> node = res.Register(CompExpr());

		if (res.HasError())
			return res;

		return res.Success(std::make_shared<UnaryOpNode>(opToken, node));
	}

	std::vector<std::string> ops = { TT_EQEQ, TT_NEQ, TT_LT, TT_GT, TT_LTEQ, TT_GTEQ };
	std::shared_ptr<Node> node = res.Register(BinOp([this]() {return ArithExpr(); }, ops));

	if (res.HasError())
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected int, float, identifier, VAR, '+', '-', '(' or 'NOT'"));

	return res.Success(node);
}

ParseResult Parser::Expr()
{
	ParseResult res;

	if (currentToken.Matches(TT_KEYWORD, "VAR"))
	{
		res.RegisterAdvancement(Advance());

		if (currentToken.GetType() != TT_IDENTIFIER)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier"));

		Token varName = currentToken;
		res.RegisterAdvancement(Advance());

		if (currentToken.GetType() != TT_EQ)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '='"));

		res.RegisterAdvancement(Advance());
		auto expr = res.Register(Expr());

		if (res.HasError())
			return res;
		else
			return res.Success(std::make_shared<VarAssignNode>(varName, expr));
	}

	std::unordered_map<std::string, std::string> ops = {
	{ TT_KEYWORD, "AND"},
	{ TT_KEYWORD, "OR"}
	};
	std::shared_ptr<Node> node = res.Register(BinOp([this]() {return CompExpr(); }, ops));

	if (res.HasError())
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected int, float, identifier, VAR, '+', '-' or ''("));

	return res.Success(node);
}

ParseResult Parser::BinOp(std::function<ParseResult()> func_a, std::vector<std::string> ops, std::function<ParseResult()> func_b)
{
	if (func_b == nullptr)
		func_b = func_a;

	ParseResult res;
	auto left = res.Register(func_a());

	if (res.HasError())
		return res;

	while (std::find(ops.begin(), ops.end(), currentToken.GetType()) != ops.end())
	{
		Token opToken = currentToken;
		res.RegisterAdvancement(Advance());
		auto right = res.Register(func_b());
		if (res.HasError())
			return res;
		left = std::make_shared<BinOpNode>(left, opToken, right);
	}

	return res.Success(left);
}

ParseResult Parser::BinOp(std::function<ParseResult()> func_a, std::unordered_map<std::string, std::string> typeValueOps, std::function<ParseResult()> func_b)
{
	if (func_b == nullptr)
		func_b = func_a;

	ParseResult res;
	auto left = res.Register(func_a());

	if (res.HasError())
		return res;

	while (
		typeValueOps.count(currentToken.GetType()) &&
		std::holds_alternative<std::string>(currentToken.GetValue()) &&
		std::get<std::string>(currentToken.GetValue()) == typeValueOps.at(currentToken.GetType())
		)
	{
		Token opToken = currentToken;
		res.RegisterAdvancement(Advance());
		auto right = res.Register(func_b());
		if (res.HasError())
			return res;
		left = std::make_shared<BinOpNode>(left, opToken, right);
	}

	return res.Success(left);
}

std::shared_ptr<Node> ParseResult::Register(const ParseResult& res)
{
	advancementCount += res.advancementCount;
	if (res.error)
		this->error = std::make_unique<Error>(*res.error);

	return res.node;
}

void ParseResult::RegisterAdvancement(const Token& token)
{
	advancementCount++;
}

ParseResult& ParseResult::Success(std::shared_ptr<Node> node)
{
	this->node = node;
	return *this;
}

ParseResult& ParseResult::Failure(std::unique_ptr<Error> error)
{
	if (error != nullptr || advancementCount == 0)
		this->error = std::move(error);
	return *this;
}
