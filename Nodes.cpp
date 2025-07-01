#include "Nodes.hpp"

NumberNode::NumberNode()
{}

NumberNode::NumberNode(Token token)
{
	this->token = token;
}

std::string NumberNode::Repr()
{
	return token.Repr();
}

BinOpNode::BinOpNode(std::shared_ptr<Node> leftNode, Token opToken, std::shared_ptr<Node> rightNode)
{
	this->leftNode = leftNode;
	this->opToken = opToken;
	this->rightNode = rightNode;
}

std::string BinOpNode::Repr()
{
	return "(" + leftNode->Repr() + ", " + opToken.Repr() + ", " + rightNode->Repr() + ")";
}

UnaryOpNode::UnaryOpNode(Token opToken, std::shared_ptr<Node> node)
{
	this->opToken = opToken;
	this->node = node;
}

std::string UnaryOpNode::Repr()
{
	return "(" + opToken.Repr() + ", " + node->Repr() + ")";
}

VarAccessNode::VarAccessNode(Token varNameTok)
{
	this->varNameTok = varNameTok;

	posStart = this->varNameTok.GetPosStart();
	posEnd = this->varNameTok.GetPosEnd();
}

std::string VarAccessNode::Repr()
{
	return "(" + varNameTok.Repr() + ")";
}

VarAssignNode::VarAssignNode(Token varNameTok, std::shared_ptr<Node> node)
{
	this->varNameTok = varNameTok;
	this->node = node;

	posStart = this->varNameTok.GetPosStart();
	posEnd = this->varNameTok.GetPosEnd();
}

std::string VarAssignNode::Repr()
{
	return "(" + varNameTok.Repr() + ", " + node->Repr() + ")";
}
