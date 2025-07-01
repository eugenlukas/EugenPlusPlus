#pragma once
#include "Token.hpp"

class Node
{
public:
	virtual std::string Repr() = 0;
	virtual ~Node() = default;
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
	Position posStart;
	Position posEnd;
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
	Position posStart;
	Position posEnd;
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