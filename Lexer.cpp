#include "Lexer.hpp"
#include <string>
#include <unordered_map>

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

	//std::cout << "Make tokens with input: '" << text << "'" << std::endl;

	std::vector<Token> tokens;

	while (current_char != '\0')
	{
		if (current_char == ' ' || current_char == '\t')
			Advance();
		else if (std::isdigit(current_char))
			tokens.push_back(makeNumber());
		else if (std::strchr(LETTERS, current_char))
			tokens.push_back(makeIdentifier());
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
				case '"':
					tokens.push_back(makeString());
					break;
			case '+':
				tokens.push_back(Token(TT_PLUS, std::nullopt, pos));
				Advance();
				break;
			case '-':
				tokens.push_back(makeMinusOrArrow());
				break;
			case '*':
				tokens.push_back(Token(TT_MUL, std::nullopt, pos));
				Advance();
				break;
			case '/':
				tokens.push_back(Token(TT_DIV, std::nullopt, pos));
				Advance();
				break;
			case '^':
				tokens.push_back(Token(TT_POW, std::nullopt, pos));
				Advance();
				break;
			case '(':
				tokens.push_back(Token(TT_LPAREN, std::nullopt, pos));
				Advance();
				break;
			case ')':
				tokens.push_back(Token(TT_RPAREN, std::nullopt, pos));
				Advance();
				break;
			case '[':
				tokens.push_back(Token(TT_LSQUARE, std::nullopt, pos));
				Advance();
				break;
			case ']':
				tokens.push_back(Token(TT_RSQUARE, std::nullopt, pos));
				Advance();
				break;
			case '!':
			{
				MakeMethodeResult res = makeNotEquals();
				if (res.error != nullptr)
					return MakeTokensResult({}, std::move(res.error));

				tokens.push_back(res.token);
				break;
			}
			case '=':
				tokens.push_back(makeEquals());
				break;
			case '<':
				tokens.push_back(makeLessThen());
				break;
			case '>':
				tokens.push_back(makeGreaterThen());
				break;
			case ',':
				tokens.push_back(Token(TT_COMMA, std::nullopt, pos));
				Advance();
				break;
			case '@':
				tokens.push_back(Token(TT_AT, std::nullopt, pos));
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

	tokens.push_back(Token(TT_EOF, std::nullopt, pos));
	return MakeTokensResult(tokens, nullptr);
}

Token Lexer::makeNumber()
{
	std::string numStr = "";
	int dotCount = 0;
	Position posStart = pos.Copy();

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

	//std::cout << numStr << std::endl;
	
	if (dotCount == 0)
		return Token(TT_INT, std::stod(numStr), posStart, pos);
	else
		return Token(TT_FLOAT, std::stod(numStr), posStart, pos);
}

Token Lexer::makeString()
{
	std::string string = "";
	Position posStart = pos.Copy();
	bool escapeCharacter = false;
	Advance();

	std::unordered_map<char, std::string> escapeCharacters = {
	{ 'n', "\n"},
	{ 't', "\t"},
	{ '"', "\""},
	{ '\\', "\\"}
	};

	while (current_char != '\0' && (current_char != '"' || escapeCharacter))
	{
		if (escapeCharacter)
		{
			if (escapeCharacters.count(current_char))
				string += escapeCharacters[current_char];
			else
				string += current_char;  // Unknown escape, keep as-is

			escapeCharacter = false;
		}
		else
		{
			if (current_char == '\\')
				escapeCharacter = true;
			else
				string += current_char;
		}
		Advance();
	}

	Advance();

	return Token(TT_STRING, string, posStart, pos);
}

Token Lexer::makeIdentifier()
{
	std::string id_str = "";
	Position posStart = pos.Copy();

	std::string letterStringUnderscore = std::string(LETTERS_DIGITS) + '_';
	while (current_char != '\0' && letterStringUnderscore.find(current_char) != std::string::npos)
	{
		id_str += current_char;
		Advance();
	}

	std::string tokType;
	if (std::find(KEYWORDS.begin(), KEYWORDS.end(), id_str) != KEYWORDS.end())
		tokType = TT_KEYWORD;
	else
		tokType = TT_IDENTIFIER;

	return Token(tokType, id_str, posStart, pos);
}

Token Lexer::makeMinusOrArrow()
{
	std::string tokType = TT_MINUS;
	Position posStart = pos.Copy();

	Advance();

	if (current_char == '>')
	{
		Advance();
		tokType = TT_ARROW;
	}

	return Token(tokType, std::nullopt, posStart, pos);
}

MakeMethodeResult Lexer::makeNotEquals()
{
	Position posStart = pos.Copy();
	Advance();

	if (current_char == '=')
	{
		Advance();
		return MakeMethodeResult(Token(TT_NEQ, std::nullopt, posStart, pos), nullptr);
	}

	Advance();
	return MakeMethodeResult({}, std::make_unique<ExpectedCharError>(posStart, pos, "'=' (after '!')"));
}

Token Lexer::makeEquals()
{
	std::string tokType = TT_EQ;

	Position posStart = pos.Copy();
	Advance();

	if (current_char == '=')
	{
		Advance();
		tokType = TT_EQEQ;
	}

	return Token(tokType, std::nullopt, posStart, pos);
}

Token Lexer::makeLessThen()
{
	std::string tokType = TT_LT;

	Position posStart = pos.Copy();
	Advance();

	if (current_char == '=')
	{
		Advance();
		tokType = TT_LTEQ;
	}

	return Token(tokType, std::nullopt, posStart, pos);
}

Token Lexer::makeGreaterThen()
{
	std::string tokType = TT_GT;

	Position posStart = pos.Copy();
	Advance();

	if (current_char == '=')
	{
		Advance();
		tokType = TT_GTEQ;
	}

	return Token(tokType, std::nullopt, posStart, pos);
}
