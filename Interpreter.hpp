#pragma once
#include <variant>
#include "Nodes.hpp"
#include "Token.hpp"
#include "Error.hpp"
#include <unordered_map>

using SymbolValue = std::variant<double, FuncDefNode>;

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
		}
		return *this;
	}
	RTResult& Success(std::optional<double> value);
	RTResult& Failure(std::unique_ptr<Error> error);

	bool HasError() const { return error != nullptr; }

	std::optional<double> GetValue() const { return value; }
	std::string GetError() const { return error->AsString(); }

	void SetValue(auto v) { value = v; }

private:
	std::unique_ptr<Error> error = nullptr;
	std::optional<double> value = std::nullopt;
};

class Interpreter
{
public:
	Interpreter(SymbolTable& symbolTable) : symbolTable(symbolTable) {}

	RTResult Visit(std::shared_ptr<Node> node);

private:
	SymbolTable& symbolTable;

	RTResult Visit_NumberNode(NumberNode& node);
	RTResult Visit_BinOpNode(BinOpNode& node);
	RTResult Visit_VarAccessNode(VarAccessNode& node);
	RTResult Visit_VarAssignNode(VarAssignNode& node);
	RTResult Visit_UnaryOpNode(UnaryOpNode& node);
	RTResult Visit_IfNode(IfNode& node);
	RTResult Visit_ForNode(ForNode& node);
	RTResult Visit_WhileNode(WhileNode& node);
	RTResult Visit_FuncDefNode(FuncDefNode& node);
	RTResult Visit_CallNode(CallNode& node);
};