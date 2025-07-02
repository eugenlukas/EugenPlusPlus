#pragma once
#include <vector>
#include <functional>
#include "Token.hpp"
#include "Nodes.hpp"
#include "Error.hpp"

class ParseResult
{
public:
	ParseResult() = default;

	ParseResult(const ParseResult& other)
	{
		if (other.error)
		{
			error = std::make_unique<Error>(*other.error);
		}
		node = other.node;
	}

	ParseResult& operator=(const ParseResult& other)
	{
		if (this != &other)
		{
			if (other.error)
			{
				error = std::make_unique<Error>(*other.error);
			}
			else
			{
				error.reset();
			}
			node = other.node;
		}
		return *this;
	}

	std::shared_ptr<Node> Register(const ParseResult& res);
	void RegisterAdvancement(const Token& token);
	ParseResult& Success(std::shared_ptr<Node> node);
	ParseResult& Failure(std::unique_ptr<Error> error);

	bool HasError() const { return error != nullptr; }

	std::shared_ptr<Node> GetNode() const { return node; }
	std::string GetError() const { return error->AsString(); }

private:
	std::unique_ptr<Error> error = nullptr;
	std::shared_ptr<Node> node = nullptr;
	int advancementCount = 0;
};

class Parser
{
public:
	Parser(std::vector<Token> tokens);

	Token Advance();

	ParseResult Parse();

	ParseResult IfExpr();
	ParseResult ForExpr();
	ParseResult WhileExpr();
	ParseResult Atom();
	ParseResult Power();
	ParseResult Factor();
	ParseResult Term();
	ParseResult ArithExpr();
	ParseResult CompExpr();
	ParseResult Expr();

	ParseResult BinOp(std::function<ParseResult()> func_a, std::vector<std::string> ops, std::function<ParseResult()> func_b=nullptr);
	ParseResult BinOp(std::function<ParseResult()> func_a, std::unordered_map<std::string, std::string> typeValueOps, std::function<ParseResult()> func_b = nullptr);

private:
	std::vector<Token> tokens;
	int tokIdx = -1;
	Token currentToken;
};