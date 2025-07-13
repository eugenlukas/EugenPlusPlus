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
	std::optional<std::shared_ptr<Node>> TryRegister(const ParseResult& res);
	void RegisterAdvancement();
	ParseResult& Success(std::shared_ptr<Node> node);
	ParseResult& Failure(std::unique_ptr<Error> error);

	bool HasError() const { return error != nullptr; }

	std::shared_ptr<Node> GetNode() const { return node; }
	std::string GetError() const { return error->AsString(); }
	Error* GetErrorPtr() { return error.get(); }
	int GetAdvancementCount() const { return advancementCount; }
	int GetToReverseCount() const { return toReverseCount; }

private:
	std::unique_ptr<Error> error = nullptr;
	std::shared_ptr<Node> node = nullptr;
	int advancementCount = 0;
	int toReverseCount = 0;
};

struct CasesResult
{
	std::vector<IfCase> cases;
	std::shared_ptr<Node> elseCase;
};

class Parser
{
public:
	Parser(std::vector<Token> tokens);

	Token Advance();
	Token Reverse(int amount=1);

	void UpdateCurrentToken();

	ParseResult Parse();

	ParseResult Statements();
	ParseResult Statement();
	ParseResult Expr();
	ParseResult CompExpr();
	ParseResult ArithExpr();
	ParseResult Term();
	ParseResult Factor();
	ParseResult Power();
	ParseResult Call();
	ParseResult Atom();
	ParseResult ListExpr();
	ParseResult IfExpr();
	ParseResult IfExprB();
	ParseResult IfExprC(std::shared_ptr<IfCase>& outElseCase);
	ParseResult IfExprBorC(CasesResult& outResult);
	ParseResult IfExprCases(std::string caseKeyword, CasesResult& outResult);
	ParseResult ForExpr();
	ParseResult WhileExpr();
	ParseResult FuncDef();

	ParseResult BinOp(std::function<ParseResult()> func_a, std::vector<std::string> ops, std::function<ParseResult()> func_b=nullptr);
	ParseResult BinOp(std::function<ParseResult()> func_a, std::vector<std::pair<std::string, std::string>> typeValueOps, std::function<ParseResult()> func_b = nullptr);

private:
	std::vector<Token> tokens;
	int tokIdx = -1;
	Token currentToken;
};