#pragma once
#include <variant>
#include "Nodes.hpp"
#include "Token.hpp"
#include "Error.hpp"
#include <unordered_map>
#include "BuildInFunctions.hpp"

struct List;
class BaseFunction;
using ListValue = std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>;
using SymbolValue = std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>;

struct List
{
	std::vector<ListValue> elements;

	List(std::vector<ListValue> elements) : elements(elements) {}
};

class SymbolTable
{
public:
	SymbolTable() = default;
	SymbolTable(SymbolTable* parent) : parent(parent) {}

	void Set(const std::string& name, const SymbolValue& value)
	{
		symbols[name] = value;
	}

	std::optional<SymbolValue> Get(const std::string& name) const
	{
		auto it = symbols.find(name);
		if (it != symbols.end())
			return it->second;
		if (parent != nullptr)
			return parent->Get(name);
		return std::nullopt;
	}

private:
	std::unordered_map<std::string, SymbolValue> symbols;
	SymbolTable* parent = nullptr;
};

class RTResult
{
public:
	RTResult() = default;

	RTResult(const RTResult& other)
	{
		if (other.error)
		{
			error = std::make_unique<Error>(*other.error);
		}
		value = other.value;
		funcReturnValue = other.funcReturnValue;
		loopShouldContinue = other.loopShouldContinue;
		loopShouldBreak = other.loopShouldBreak;
	}

	RTResult& operator=(const RTResult& other)
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

			value = other.value;
			funcReturnValue = other.funcReturnValue;
			loopShouldContinue = other.loopShouldContinue;
			loopShouldBreak = other.loopShouldBreak;
		}
		return *this;
	}
	RTResult& Success(std::optional<SymbolValue> value);
	RTResult& SuccessReturn(std::optional<SymbolValue> value);
	RTResult& SuccessContinue();
	RTResult& SuccessBreak();

	RTResult& Failure(std::unique_ptr<Error> error);

	bool ShouldReturn();

	void Reset();

	bool HasError() const { return error != nullptr; }
	std::string GetError() const { return error->AsString(); }

	std::optional<SymbolValue> GetValue() const { return value; }
	void SetValue(std::optional<SymbolValue> v) { value = v; }

	std::optional<SymbolValue> GetFuncReturnValue() { return funcReturnValue; }
	void SetFuncReturnValue(std::optional<SymbolValue> v) { funcReturnValue = v; }

	bool GetLoopShouldContinue() { return loopShouldContinue; }
	void SetLoopShouldContinue(bool v) { loopShouldContinue = v; }

	bool GetLoopShouldBreak() { return loopShouldBreak; }
	void SetLoopShouldBreak(bool v) { loopShouldBreak = v; }

private:
	std::unique_ptr<Error> error = nullptr;
	std::optional<SymbolValue> value = std::nullopt;
	std::optional<SymbolValue> funcReturnValue = std::nullopt;
	bool loopShouldContinue = false;
	bool loopShouldBreak = false;
};

class Interpreter
{
public:
	Interpreter(SymbolTable& symbolTable) : symbolTable(symbolTable) {}

	RTResult Visit(std::shared_ptr<Node> node);

private:
	SymbolTable& symbolTable;

	RTResult Visit_NumberNode(NumberNode& node);
	RTResult Visit_StringNode(StringNode& node);
	RTResult Visit_ListNode(ListNode& node);
	RTResult Visit_BinOpNode(BinOpNode& node);
	RTResult Visit_VarAccessNode(VarAccessNode& node);
	RTResult Visit_VarAssignNode(VarAssignNode& node);
	RTResult Visit_UnaryOpNode(UnaryOpNode& node);
	RTResult Visit_IfNode(IfNode& node);
	RTResult Visit_ForNode(ForNode& node);
	RTResult Visit_WhileNode(WhileNode& node);
	RTResult Visit_FuncDefNode(FuncDefNode& node);
	RTResult Visit_CallNode(CallNode& node);
	RTResult Visit_ReturnNode(ReturnNode& node);
	RTResult Visit_ContinueNode(ContinueNode& node);
	RTResult Visit_BreakNode(BreakNode& node);
};