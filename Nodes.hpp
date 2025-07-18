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
	IfCase() = default;
	IfCase(std::shared_ptr<Node> condition, std::shared_ptr<Node> expr, bool shouldReturnNull) : condition(condition), expr(expr), shouldReturnNull(shouldReturnNull) {}

	std::shared_ptr<Node> GetCondition() { return condition; }
	std::shared_ptr<Node> GetExpr() { return expr; }
	bool GetShouldReturnNull() const { return shouldReturnNull; }

private:
	std::shared_ptr<Node> condition;
	std::shared_ptr<Node> expr;
	bool shouldReturnNull;
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

class StringNode : public Node
{
public:
	StringNode();
	StringNode(Token token);

	std::string Repr() override;
	Token GetToken() { return token; };

private:
	Token token;
};

class ListNode : public Node 
{
public:
	ListNode();
	ListNode(std::vector<std::shared_ptr<Node>> elementNodes, Position posStart, Position posEnd);

	std::string Repr() override;
	std::vector<std::shared_ptr<Node>> GetElementNodes() { return elementNodes; }

private:
	std::vector<std::shared_ptr<Node>> elementNodes;
};

class VarAccessNode : public Node
{
public:
	VarAccessNode(Token varNameTok, std::optional<std::string> moduleAlias);

	std::string Repr() override;
	Token GetVarNameToken() { return varNameTok; }
	Position GetPosStart() { return posStart; }
	Position GetPosEnd() { return posEnd; }
	std::optional<std::string> GetModuleAlias() { return moduleAlias; }

	bool IsNamespaced() const { return moduleAlias.has_value(); }

private:
	Token varNameTok;
	std::optional<std::string> moduleAlias;
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

class ForNode : public Node
{
public:
	ForNode(Token varNameTok, std::shared_ptr<Node> startValueNode, std::shared_ptr<Node> endValueNode, std::shared_ptr<Node> stepValueNode=nullptr, std::shared_ptr<Node> bodyNode=nullptr, bool shouldReturnNull=false);

	std::string Repr() override;
	Token GetVarNameTok() { return varNameTok; }
	std::shared_ptr<Node> GetStartValueNode() { return startValueNode; }
	std::shared_ptr<Node> GetEndValueNode() { return endValueNode; }
	std::shared_ptr<Node> GetStepValueNode() { return stepValueNode; }
	std::shared_ptr<Node> GetBodyNode() { return bodyNode; }
	bool GetShouldReturnNull() const { return shouldReturnNull; }

private:
	Token varNameTok;
	std::shared_ptr<Node> startValueNode;
	std::shared_ptr<Node> endValueNode;
	std::shared_ptr<Node> stepValueNode;
	std::shared_ptr<Node> bodyNode;
	bool shouldReturnNull;
};

class WhileNode : public Node
{
public:
	WhileNode(std::shared_ptr<Node> conditionNode, std::shared_ptr<Node> bodyNode, bool shouldReturnNull);

	std::string Repr() override;
	std::shared_ptr<Node> GetConditionNode() { return conditionNode; }
	std::shared_ptr<Node> GetBodyNode() { return bodyNode; }
	bool GetShouldReturnNull() const { return shouldReturnNull; }

private:
	std::shared_ptr<Node> conditionNode;
	std::shared_ptr<Node> bodyNode;
	bool shouldReturnNull;
};

class FuncDefNode : public Node
{
public:
	FuncDefNode(std::optional<Token> varNameTok, std::vector<Token> argNameToks, std::shared_ptr<Node> bodyNode, bool shouldAutoReturn);

	std::string Repr() override;
	std::optional<Token> GetVarNameTok() { return varNameTok; }
	std::vector<Token> GetArgNameToks() { return argNameToks; }
	std::shared_ptr<Node> GetBodyNode() { return bodyNode; }
	bool GetShouldAutoReturn() const { return shouldAutoReturn; }

private:
	std::optional<Token> varNameTok;
	std::vector<Token> argNameToks;
	std::shared_ptr<Node> bodyNode;
	bool shouldAutoReturn;
};

class CallNode : public Node
{
public:
	CallNode(std::shared_ptr<Node> nodeToCall, std::vector<std::shared_ptr<Node>> argNodes);

	std::string Repr() override;
	std::shared_ptr<Node> GetNodeToCall() { return nodeToCall; }
	std::vector<std::shared_ptr<Node>> GetArgNodes() { return argNodes; }

private:
	std::shared_ptr<Node> nodeToCall;
	std::vector<std::shared_ptr<Node>> argNodes;
};

class ReturnNode : public Node
{
public:
	ReturnNode(std::optional<std::shared_ptr<Node>> nodeToReturn, Position posStart, Position posEnd);

	std::string Repr() override;
	std::optional<std::shared_ptr<Node>> GetNodeToReturn() { return nodeToReturn; }

private:
	std::optional<std::shared_ptr<Node>> nodeToReturn;
};

class ContinueNode : public Node
{
public:
	ContinueNode(Position posStart, Position posEnd);

	std::string Repr() override;
};

class BreakNode : public Node
{
public:
	BreakNode(Position posStart, Position posEnd);

	std::string Repr() override;
};

class ImportNode : public Node
{
public:
	ImportNode(Token filepathToken, std::string alias, Position posStart, Position posEnd);

	std::string Repr() override;
	Token GetFilepathToken() { return filepathToken; }
	std::string GetAlias() { return alias; }

private:
	Token filepathToken;
	std::string alias;
};