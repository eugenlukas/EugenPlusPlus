#include "Nodes.hpp"

NumberNode::NumberNode()
{}

NumberNode::NumberNode(Token token)
{
	this->token = token;

	posStart = token.GetPosStart();
	posEnd = token.GetPosEnd();
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

IfNode::IfNode(std::vector<IfCase> cases, std::shared_ptr<Node> elseCase)
{
	this->cases = cases;
	this->elseCase = elseCase;

	if (!cases.empty())
	{
		posStart = cases[0].GetCondition()->GetPosStart();

		if (elseCase != nullptr)
		{
			posEnd = elseCase->GetPosEnd();
		}
		else
		{
			posEnd = cases.back().GetCondition()->GetPosEnd();
		}
	}
}

std::string IfNode::Repr()
{
	std::string repr = "(IF ";

	for (auto& ifCase : cases)
	{
		repr += "[Cond: " + ifCase.GetCondition()->Repr();
		repr += ", Expr: " + ifCase.GetExpr()->Repr() + "] ";
	}

	if (elseCase != nullptr)
	{
		repr += "ELSE: " + elseCase->Repr();
	}

	repr += ")";
	return repr;
}

ForNode::ForNode(Token varNameTok, std::shared_ptr<Node> startValueNode, std::shared_ptr<Node> endValueNode, std::shared_ptr<Node> stepValueNode, std::shared_ptr<Node> bodyNode)
{
	this->varNameTok = varNameTok;
	this->startValueNode = startValueNode;
	this->endValueNode = endValueNode;
	this->startValueNode = startValueNode;
	this->bodyNode = bodyNode;

	posStart = varNameTok.GetPosStart();
	posEnd = varNameTok.GetPosEnd();
}

std::string ForNode::Repr()
{
	return std::string();
}

WhileNode::WhileNode(std::shared_ptr<Node> conditionNode, std::shared_ptr<Node> bodyNode)
{
	this->conditionNode = conditionNode;
	this->bodyNode = bodyNode;

	posStart = conditionNode->GetPosStart();
	posEnd = bodyNode->GetPosEnd();
}

std::string WhileNode::Repr()
{
	return std::string();
}
