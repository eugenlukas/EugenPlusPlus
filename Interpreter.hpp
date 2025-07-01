#pragma once
#include <variant>
#include "Nodes.hpp"
#include "Token.hpp"
#include "Error.hpp"
#include <unordered_map>

class SymbolTable
{
public:
	SymbolTable() = default;
	SymbolTable(SymbolTable* parent) : parent(parent) {}

	void Set(const std::string& name, double value);

	std::optional<double> Get(const std::string& name) const;

	bool Remove(const std::string& name);

private:
	std::unordered_map<std::string, double> symbols;
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
	RTResult Visit_VarAccsessNode(VarAccessNode& node);
	RTResult Visit_VarAssignNode(VarAssignNode& node);
	RTResult Visit_UnaryOpNode(UnaryOpNode& node);
	RTResult Visit_IfNode(IfNode& node);
};