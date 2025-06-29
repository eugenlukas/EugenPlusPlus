#pragma once
#include <iostream>

constexpr char TT_INT[] = "TT_INT";
constexpr char TT_FLOAT[] = "FLOAT";
constexpr char TT_PLUS[] = "PLUS";
constexpr char TT_MINUS[] = "MINUS";
constexpr char TT_MUL[] = "MUL";
constexpr char TT_DIV[] = "DIV";
constexpr char TT_LPAREN[] = "LPAREN";
constexpr char TT_RPAREN[] = "RPAREN";
constexpr char DIGITS[] = "0123456789";

class Token
{
public:
	Token(const std::string& type_, const std::string& value_);

	std::string Repr();

private:
	std::string type;
	std::string value = "";
};