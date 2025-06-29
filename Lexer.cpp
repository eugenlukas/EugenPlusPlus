#include "Lexer.hpp"

Lexer::Lexer(const std::string& fn, const std::string& text)
{
	this->fn = fn;
	this->text = text;
	this->pos = Position(-1, 0, -1, fn, text);
	Advance();
}

void Lexer::Advance()
{
	pos.Advance(current_char);
	if (pos.GetIdx() < text.length())
		current_char = text[pos.GetIdx()];
	else
		current_char = '\0';
}

MakeTokensResult Lexer::MakeTokens()
{
	std::vector<Token> tokens;

	while (current_char != '\0')
	{
		if (current_char == ' ' || current_char == '\t')
			Advance();
		else if (std::isdigit(current_char))
			tokens.push_back(makeNumber());
		else
		{
			switch (current_char)
			{
			case ' ':
				Advance();
				break;
			case '\t':
				Advance();
				break;
			case '+':
				tokens.push_back(Token(TT_PLUS, ""));
				Advance();
				break;
			case '-':
				tokens.push_back(Token(TT_MINUS, ""));
				Advance();
				break;
			case '*':
				tokens.push_back(Token(TT_MUL, ""));
				Advance();
				break;
			case '/':
				tokens.push_back(Token(TT_DIV, ""));
				Advance();
				break;
			case '(':
				tokens.push_back(Token(TT_LPAREN, ""));
				Advance();
				break;
			case ')':
				tokens.push_back(Token(TT_RPAREN, ""));
				Advance();
				break;
			default:
				Position pos_start = pos.Copy();
				char illegalChar = current_char;
				Advance();
				return MakeTokensResult({}, std::make_unique<IllegalCharError>(pos_start, pos, "'" + std::string(1, illegalChar) + "'"));
				break;
			}
		}
	}

	return MakeTokensResult(tokens, nullptr);
}

Token Lexer::makeNumber()
{
	std::string numStr = "";
	int dotCount = 0;

	while (current_char != '\0' && (std::isdigit(current_char) || current_char == '.'))
	{
		if (current_char == '.')
		{
			if (dotCount >= 1) break;

			dotCount++;
			numStr += '.';
		}
		else
			numStr += current_char;
			
		Advance();
	}
	
	if (dotCount == 0)
		return Token(TT_INT, numStr);
	else
		return Token(TT_FLOAT, numStr);
}
