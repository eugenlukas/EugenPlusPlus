#pragma once
#include "Token.hpp"
#include <vector>

class Node
{
public:
	virtual std::string Repr() = 0;
	virtual Position GetPosStart() { return posStart; }
	virtual Position GetPosEnd() { return posEnd; }
	virtual ~Node() = default;

protected:
	Position posStart;
	Position posEnd;
};

class IfCase
{
public:
	IfCase();
	IfCase(std::shared_ptr<Node> condition, std::shared_ptr<Node> expr) : condition(condition), expr(expr) {}

	std::shared_ptr<Node> GetCondition() { return condition; }
	std::shared_ptr<Node> GetExpr() { return expr; }

private:
	std::shared_ptr<Node> condition;
	std::shared_ptr<Node> expr;
};

class NumberNode : public Node
{
public:
	NumberNode();
	NumberNode(Token token);

	std::string Repr() override;
	Token GetToken() { return token; };

private:
	Token token;
};

class VarAccessNode : public Node
{
public:
	VarAccessNode(Token varNameTok);

	std::string Repr() override;
	Token GetVarNameToken() { return varNameTok; }
	Position GetPosStart() { return posStart; }
	Position GetPosEnd() { return posEnd; }

private:
	Token varNameTok;
};

class VarAssignNode : public Node
{
public:
	VarAssignNode(Token varNameTok, std::shared_ptr<Node> node);

	std::string Repr() override;
	Token GetVarNameToken() { return varNameTok; }
	std::shared_ptr<Node> GetValueNode() { return node; }

private:
	Token varNameTok;
	std::shared_ptr<Node> node;
};

class BinOpNode : public Node
{
public:
	BinOpNode(std::shared_ptr<Node> leftNode, Token opToken, std::shared_ptr<Node> rightNode);

	std::string Repr() override;
	std::shared_ptr<Node> GetLeftNode() { return leftNode; };
	Token GetOpToken() { return opToken; };
	std::shared_ptr<Node> GetRightNode() { return rightNode; };

private:
	std::shared_ptr<Node> leftNode;
	Token opToken;
	std::shared_ptr<Node> rightNode;
};

class UnaryOpNode : public Node
{
public:
	UnaryOpNode(Token opToken, std::shared_ptr<Node> node);

	std::string Repr() override;
	Token GetOpToken() { return opToken; }
	std::shared_ptr<Node> GetNode() { return node; }

private:
	Token opToken;
	std::shared_ptr<Node> node;
};

class IfNode : public Node
{
public:
	IfNode(std::vector<IfCase> cases, std::shared_ptr<Node> elseCase);

	std::string Repr() override;
	std::vector<IfCase> GetCases() { return cases; }
	std::shared_ptr<Node> GetElseCase() { return elseCase; }

private:
	std::vector<IfCase> cases;
	std::shared_ptr<Node> elseCase;
};