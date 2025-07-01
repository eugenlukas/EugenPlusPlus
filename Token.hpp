#pragma once
#include <iostream>
#include "Position.hpp"
#include <optional>
#include <variant>
#include <string>
#include <array>

constexpr char TT_INT[]			= "INT";
constexpr char TT_FLOAT[]		= "FLOAT";
constexpr char TT_IDENTIFIER[]	= "IDENTIFIER";
constexpr char TT_KEYWORD[]		= "KEYWORD";
constexpr char TT_PLUS[]		= "PLUS";
constexpr char TT_MINUS[]		= "MINUS";
constexpr char TT_MUL[]			= "MUL";
constexpr char TT_DIV[]			= "DIV";
constexpr char TT_POW[]			= "POW";
constexpr char TT_EQ[]			= "EQ";
constexpr char TT_LPAREN[]		= "LPAREN";
constexpr char TT_RPAREN[]		= "RPAREN";
constexpr char TT_EOF[]			= "EOF";

constexpr char DIGITS[]			= "0123456789";
constexpr char LETTERS[]		= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr char LETTERS_DIGITS[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

const std::array<std::string, 1> KEYWORDS = { "VAR" };

class Token
{
public:
	Token();
	Token(const std::string& type_, std::optional<std::variant<int, double, std::string>> value = std::nullopt, std::optional<Position> posStart = std::nullopt, std::optional<Position> posEnd = std::nullopt);

	std::string Repr();

	bool Matches(std::string type_, std::optional<std::variant<int, double, std::string>> value);

	std::string GetType() { return type; }
	Position GetPosStart() { return posStart.value(); }
	Position GetPosEnd() { return posEnd.value(); }

	std::variant<int, double, std::string> GetValue() const;

private:
	std::string type;
	std::optional<std::variant<int, double, std::string>> value = std::nullopt;
	std::optional<Position> posStart = std::nullopt;
	std::optional<Position> posEnd = std::nullopt;
};