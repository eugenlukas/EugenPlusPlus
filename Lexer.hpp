#pragma once
#include <iostream>
#include <vector>
#include "Token.hpp"
#include "Error.hpp"
#include "Position.hpp"

typedef struct MakeTokensResult
{
	MakeTokensResult(const std::vector<Token>& tokens, std::unique_ptr<Error> error)
		: tokens(tokens), error(std::move(error))
	{ }

	std::vector<Token> tokens;
	std::unique_ptr<Error> error;
} MakeTokensResult;

typedef struct MakeMethodeResult {
	MakeMethodeResult(const Token& token, std::unique_ptr<Error> error)
		: token(token), error(std::move(error))
	{ }

	Token token;
	std::unique_ptr<Error> error;
};

class Lexer
{
public:
	Lexer(const std::string& fn, const std::string& text);

	void Advance();
	MakeTokensResult MakeTokens();

private:
	Token makeNumber();
	Token makeString();
	Token makeIdentifier();
	Token makeMinusOrArrow();
	MakeMethodeResult makeNotEquals();
	Token makeEquals();
	Token makeLessThen();
	Token makeGreaterThen();

private:
	std::string fn;
	std::string text;
	Position pos;
	char current_char = '\0';

};