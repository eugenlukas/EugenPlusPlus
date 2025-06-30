#pragma once
#include <variant>
#include "Nodes.hpp"
#include "Token.hpp"
#include "Error.hpp"

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
	RTResult& Success(double value);
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
	RTResult Visit(std::shared_ptr<Node> node);

private:
	RTResult Visit_NumberNode(NumberNode& node);
	RTResult Visit_BinOpNode(BinOpNode& node);
	RTResult Visit_UnaryOpNode(UnaryOpNode& node);
};