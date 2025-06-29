#pragma once
#include <iostream>
#include "Position.hpp"
#include <optional>

constexpr char TT_INT[] = "INT";
constexpr char TT_FLOAT[] = "FLOAT";
constexpr char TT_PLUS[] = "PLUS";
constexpr char TT_MINUS[] = "MINUS";
constexpr char TT_MUL[] = "MUL";
constexpr char TT_DIV[] = "DIV";
constexpr char TT_LPAREN[] = "LPAREN";
constexpr char TT_RPAREN[] = "RPAREN";
constexpr char TT_EOF[] = "EOF";
constexpr char DIGITS[] = "0123456789";

class Token
{
public:
	Token();
	Token(const std::string& type_, const std::string& value="", std::optional<Position> posStart=std::nullopt, std::optional<Position> posEnd=std::nullopt);

	std::string Repr();

	std::string GetType() { return type; }
	Position GetPosStart() { return posStart.value(); }
	Position GetPosEnd() { return posEnd.value(); }

private:
	std::string type;
	std::string value = "";
	std::optional<Position> posStart = std::nullopt;
	std::optional<Position> posEnd = std::nullopt;
};