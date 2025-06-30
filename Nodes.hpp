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