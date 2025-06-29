#include "Token.hpp"

Token::Token()
{}

Token::Token(const std::string & type_, const std::string & value, std::optional<Position> posStart, std::optional<Position> posEnd)
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
	if (!value.empty())
		return type + ":" + value;
	else
		return type;
}
