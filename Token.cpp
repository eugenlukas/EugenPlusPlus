#include "Token.hpp"

Token::Token()
{}

Token::Token(const std::string & type_, std::optional<std::variant<int, double, std::string>> value, std::optional<Position> posStart, std::optional<Position> posEnd)
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
		else if (std::holds_alternative<double>(value.value()))
			return "FLOAT:" + std::to_string(std::get<double>(value.value()));
		else if (std::holds_alternative<std::string>(value.value()))
			return type + ":" + std::get<std::string>(value.value());
	}
	return type;
}

bool Token::Matches(std::string type_, std::optional<std::variant<int, double, std::string>> value)
{
	//std::cout << "Type1: " << type << " zu Type2: " << type_ << std::endl;

	if (!type.compare(type_))
		if (this->value = value)
			return true;
		else
		{
			return false;
		}
	else
	{
		return false;
	}
}

std::variant<int, double, std::string> Token::GetValue() const
{
	if (value.has_value())
		return value.value();
	throw std::runtime_error("Token has no value");
}
