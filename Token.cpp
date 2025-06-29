#include "Token.hpp"

Token::Token(const std::string& type_, const std::string& value_)
{
	type = type_;
	value = value_;
}

std::string Token::Repr()
{
	if (!value.empty())
		return type + ":" + value;
	else
		return type;
}
