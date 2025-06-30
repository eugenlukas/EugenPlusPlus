#include "Token.hpp"
#include <string>

Token::Token()
{}

Token::Token(const std::string & type_, std::optional<std::variant<int, double>> value, std::optional<Position> posStart, std::optional<Position> posEnd)
{
	type = type_;
	this->value = value;

	if (posStart.has_value())
	{
		this->posStart = posStart->Copy();
		this->posEnd = posStart->Copy();
		this->posEnd->Advance();
	}

	if (posEnd.has_value())
		this->posEnd = posEnd;
}

std::string Token::Repr()
{
	if (value.has_value())
	{
		if (std::holds_alternative<int>(value.value()))
			return "INT:" + std::to_string(std::get<int>(value.value()));
		else
			return "FLOAT:" + std::to_string(std::get<double>(value.value()));
	}
	return type;
}

std::variant<int, double> Token::GetValue() const
{
	if (value.has_value())
		return value.value();
	throw std::runtime_error("Token has no value");
}
