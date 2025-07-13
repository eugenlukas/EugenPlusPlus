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

StringNode::StringNode()
{}

StringNode::StringNode(Token token)
{
	this->token = token;

	posStart = token.GetPosStart();
	posEnd = token.GetPosEnd();
}

std::string StringNode::Repr()
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
		if (cases[0].GetCondition())
			posStart = cases[0].GetCondition()->GetPosStart();
		else if (cases[0].GetExpr())
			posStart = cases[0].GetExpr()->GetPosStart();

		if (elseCase != nullptr)
		{
			posEnd = elseCase->GetPosEnd();
		}
		else
		{
			// Backward search for last available condition/expr
			for (int i = static_cast<int>(cases.size()) - 1; i >= 0; --i)
			{
				if (cases[i].GetExpr())
				{
					posEnd = cases[i].GetExpr()->GetPosEnd();
					break;
				}
				else if (cases[i].GetCondition())
				{
					posEnd = cases[i].GetCondition()->GetPosEnd();
					break;
				}
			}
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

ForNode::ForNode(Token varNameTok, std::shared_ptr<Node> startValueNode, std::shared_ptr<Node> endValueNode, std::shared_ptr<Node> stepValueNode, std::shared_ptr<Node> bodyNode, bool shouldReturnNull)
{
	this->varNameTok = varNameTok;
	this->startValueNode = startValueNode;
	this->endValueNode = endValueNode;
	this->stepValueNode = stepValueNode;
	this->bodyNode = bodyNode;
	this->shouldReturnNull = shouldReturnNull;

	posStart = varNameTok.GetPosStart();
	posEnd = varNameTok.GetPosEnd();
}

std::string ForNode::Repr()
{
	return std::string();
}

WhileNode::WhileNode(std::shared_ptr<Node> conditionNode, std::shared_ptr<Node> bodyNode, bool shouldReturnNull)
{
	this->conditionNode = conditionNode;
	this->bodyNode = bodyNode;
	this->shouldReturnNull = shouldReturnNull;

	posStart = conditionNode->GetPosStart();
	posEnd = bodyNode->GetPosEnd();
}

std::string WhileNode::Repr()
{
	return std::string();
}

FuncDefNode::FuncDefNode(std::optional<Token> varNameTok, std::vector<Token> argNameToks, std::shared_ptr<Node> bodyNode, bool shouldAutoReturn)
{
	this->varNameTok = varNameTok;
	this->argNameToks = argNameToks;
	this->bodyNode = bodyNode;
	this->shouldAutoReturn = shouldAutoReturn;

	if (varNameTok.has_value())
		posStart = varNameTok.value().GetPosStart();
	else if (argNameToks.size() > 0)
		posStart = argNameToks[0].GetPosStart();
	else
		posStart = bodyNode->GetPosStart();

	posEnd = bodyNode->GetPosEnd();
}

std::string FuncDefNode::Repr()
{
	return "<function '" + std::get<std::string>(varNameTok.value().GetValue()) + "'>";
}

CallNode::CallNode(std::shared_ptr<Node> nodeToCall, std::vector<std::shared_ptr<Node>> argNodes)
{
	this->nodeToCall = nodeToCall;
	this->argNodes = argNodes;

	posStart = nodeToCall->GetPosStart();

	if (argNodes.size() > 0)
		posEnd = argNodes.back()->GetPosEnd();
	else
		posEnd = nodeToCall->GetPosEnd();
}

std::string CallNode::Repr()
{
	return std::string();
}

ListNode::ListNode()
{}

ListNode::ListNode(std::vector<std::shared_ptr<Node>> elementNodes, Position posStart, Position posEnd)
{
	this->elementNodes = elementNodes;
	this->posStart = posStart;
	this->posEnd = posEnd;
}

std::string ListNode::Repr()
{
	std::string result = "[";
	for (size_t i = 0; i < elementNodes.size(); ++i)
	{
		result += elementNodes[i] ? elementNodes[i]->Repr() : "null";
		if (i != elementNodes.size() - 1)
		{
			result += ", ";
		}
	}
	result += "]";
	return result;
}

ReturnNode::ReturnNode(std::optional<std::shared_ptr<Node>> nodeToReturn, Position posStart, Position posEnd)
{
	this->nodeToReturn = nodeToReturn;
	this->posStart = posStart;
	this->posEnd = posEnd;
}

std::string ReturnNode::Repr()
{
	return std::string();
}

ContinueNode::ContinueNode(Position posStart, Position posEnd)
{
	this->posStart = posStart;
	this->posEnd = posEnd;
}

std::string ContinueNode::Repr()
{
	return std::string();
}

BreakNode::BreakNode(Position posStart, Position posEnd)
{
	this->posStart = posStart;
	this->posEnd = posEnd;
}

std::string BreakNode::Repr()
{
	return std::string();
}
