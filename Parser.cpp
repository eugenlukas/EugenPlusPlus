#include "Parser.hpp"
#include <algorithm>

Parser::Parser(std::vector<Token> tokens)
{
	this->tokens = tokens;
	Advance();
}

Token Parser::Advance()
{
	tokIdx++;
	UpdateCurrentToken();

	return currentToken;
}

Token Parser::Reverse(int amount)
{
	tokIdx -= amount;
	UpdateCurrentToken();

	return currentToken;
}

void Parser::UpdateCurrentToken()
{
	if (tokIdx >= 0 && tokIdx < tokens.size())
		currentToken = tokens[tokIdx];
}

ParseResult Parser::Parse()
{
	//std::cout << "Parse!" << std::endl;
	ParseResult res = Statements();
	if (!res.HasError() && currentToken.GetType() != TT_EOF)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '+', '-', '*' or '/'"));
	return res;
}

ParseResult Parser::Statements()
{
	ParseResult res;
	std::vector<std::shared_ptr<Node>> statements;
	Position posStart = currentToken.GetPosStart().Copy();

	while (currentToken.GetType() == TT_NEWLINE)
	{
		Advance();
		res.RegisterAdvancement();
	}

	std::optional<std::shared_ptr<Node>> statement = res.Register(Statement());
	if (res.HasError())
		return res;

	statements.push_back(statement.value());

	bool moreStatements = true;

	while (true)
	{
		int newLineCount = 0;

		while (currentToken.GetType() == TT_NEWLINE)
		{
			Advance();
			res.RegisterAdvancement();
			newLineCount++;
		}
		if (newLineCount == 0)
			moreStatements = false;

		if (!moreStatements)
			break;

		statement = res.TryRegister(Statement());
		if (!statement.has_value())
		{
			Reverse(res.GetToReverseCount());
			moreStatements = false;
			continue;
		}

		statements.push_back(statement.value());
	}

	if (statements.size() == 1)
		return res.Success(statements[0]);
	else
		return res.Success(std::make_unique<ListNode>(statements, posStart, currentToken.GetPosEnd().Copy()));
}

ParseResult Parser::Statement()
{
	ParseResult res;
	Position posStart = currentToken.GetPosStart().Copy();

	if (currentToken.Matches(TT_KEYWORD, "RETURN"))
	{
		Advance();
		res.RegisterAdvancement();

		std::optional<std::shared_ptr<Node>> expr = res.TryRegister(Expr());
		if (!expr.has_value())
			Reverse(res.GetToReverseCount());
		return res.Success(std::make_unique<ReturnNode>(expr, posStart, currentToken.GetPosEnd().Copy()));
	}

	if (currentToken.Matches(TT_KEYWORD, "CONTINUE"))
	{
		Advance();
		res.RegisterAdvancement();

		return res.Success(std::make_unique<ContinueNode>(posStart, currentToken.GetPosEnd().Copy()));
	}

	if (currentToken.Matches(TT_KEYWORD, "BREAK"))
	{
		Advance();
		res.RegisterAdvancement();

		return res.Success(std::make_unique<BreakNode>(posStart, currentToken.GetPosEnd().Copy()));
	}

	if (currentToken.GetType() == TT_HASH)
	{
		std::shared_ptr<Node> importStatement = res.Register(ImportStatement());
		if (res.HasError())
			return res;

		return res.Success(importStatement);
	}

	std::shared_ptr<Node> expr = res.Register(Expr());
	if (res.HasError())
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected int, float, identifier, VAR, '+', '-', '(', '[', 'IF', 'FOR', 'WHILE', 'FUNC', 'RETURN', 'CONTINUE', 'BREAK' or NOT"));

	return res.Success(expr);
}

ParseResult Parser::ImportStatement()
{
	ParseResult res;
	Position posStart = currentToken.GetPosStart().Copy();

	Advance();
	res.RegisterAdvancement();

	if (!currentToken.Matches(TT_KEYWORD, "IMPORT"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'IMPORT'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_STRING)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected string for file path"));

	Token filepathToken = currentToken;

	Advance();
	res.RegisterAdvancement();

	if (!currentToken.Matches(TT_KEYWORD, "AS"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'AS'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_IDENTIFIER)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier as alias"));

	std::string alias = std::get<std::string>(currentToken.GetValue());

	Advance();
	res.RegisterAdvancement();

	return res.Success(std::make_unique<ImportNode>(filepathToken, alias, posStart, currentToken.GetPosEnd().Copy()));
}

ParseResult Parser::Expr()
{
	ParseResult res;

	if (currentToken.Matches(TT_KEYWORD, "VAR"))
	{
		Advance();
		res.RegisterAdvancement();

		if (currentToken.GetType() != TT_IDENTIFIER)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier"));

		Token varName = currentToken;

		Advance();
		res.RegisterAdvancement();

		if (currentToken.GetType() != TT_EQ)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '='"));

		Advance();
		res.RegisterAdvancement();
		auto expr = res.Register(Expr());

		if (res.HasError())
			return res;
		else
			return res.Success(std::make_shared<VarAssignNode>(varName, expr));
	}

	std::vector<std::pair<std::string, std::string>> ops = {
	{ TT_KEYWORD, "AND"},
	{ TT_KEYWORD, "OR"}
	};
	std::shared_ptr<Node> node = res.Register(BinOp([this]() {return CompExpr(); }, ops));

	if (res.HasError())
	{
		if (res.GetAdvancementCount() == 0 && !res.GetErrorPtr())
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(),"Expected int, float, identifier, VAR, '+', '-', '(', '[', 'IF', 'FOR', 'WHILE', 'FUNC' or NOT"));
		return res;
	}

	return res.Success(node);
}

ParseResult Parser::CompExpr()
{
	ParseResult res;

	if (currentToken.Matches(TT_KEYWORD, "NOT"))
	{
		Token opToken = currentToken;

		Advance();
		res.RegisterAdvancement();

		std::shared_ptr<Node> node = res.Register(CompExpr());

		if (res.HasError())
			return res;

		return res.Success(std::make_shared<UnaryOpNode>(opToken, node));
	}

	std::vector<std::string> ops = { TT_EQEQ, TT_NEQ, TT_LT, TT_GT, TT_LTEQ, TT_GTEQ };
	std::shared_ptr<Node> node = res.Register(BinOp([this]() {return ArithExpr(); }, ops));

	if (res.HasError())
	{
		if (res.GetAdvancementCount() == 0 && !res.GetErrorPtr())
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected int, float, identifier, VAR, '+', '-', '(', '[' or 'NOT'"));
		return res;
	}

	return res.Success(node);
}

ParseResult Parser::ArithExpr()
{
	std::vector<std::string> ops = { TT_PLUS, TT_MINUS, TT_AT };
	return BinOp([this]() {return Term(); }, ops);
}

ParseResult Parser::Term()
{
	std::vector<std::string> ops = { TT_MUL, TT_DIV };
	return BinOp([this]() {return Factor(); }, ops);
}

ParseResult Parser::Factor()
{
	ParseResult res = ParseResult();
	Token tok = currentToken;

	if (tok.GetType() == TT_PLUS || tok.GetType() == TT_MINUS)
	{
		Advance();
		res.RegisterAdvancement();
		std::shared_ptr<Node> factor = res.Register(Factor());
		if (res.HasError())
			return res;
		return res.Success(std::make_shared<UnaryOpNode>(tok, factor));
	}

	return Power();
}

ParseResult Parser::Power()
{
	std::vector<std::string> ops = { TT_POW };
	return BinOp([this]() {return Call(); }, ops, [this]() {return Factor(); });
}

ParseResult Parser::Call()
{
	ParseResult res;

	std::shared_ptr<Node> atom = res.Register(Atom());
	if (res.HasError())
		return res;

	if (currentToken.GetType() == TT_LPAREN)
	{
		Advance();
		res.RegisterAdvancement();
		
		std::vector<std::shared_ptr<Node>> argNodes;

		if (currentToken.GetType() == TT_RPAREN)
		{
			Advance();
			res.RegisterAdvancement();
		}
		else
		{
			argNodes.push_back(res.Register(Expr()));
			if (res.HasError())
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ')', 'VAR', 'IF', 'FOR', 'WHILE', 'FUNC', 'int', 'float', identifier, '+', '-', '(', '[' or 'NOT'"));

			while (currentToken.GetType() == TT_COMMA)
			{
				Advance();
				res.RegisterAdvancement();

				argNodes.push_back(res.Register(Expr()));
				if (res.HasError())
					return res;
			}

			if (currentToken.GetType() != TT_RPAREN)
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ',' or ')'"));

			Advance();
			res.RegisterAdvancement();
		}

		return res.Success(std::make_shared<CallNode>(atom, argNodes));
	}

	return res.Success(atom);
}

ParseResult Parser::Atom()
{
	ParseResult res = ParseResult();
	Token tok = currentToken;

	if (tok.GetType() == TT_INT || tok.GetType() == TT_FLOAT)
	{
		Advance();
		res.RegisterAdvancement();
		return res.Success(std::make_shared<NumberNode>(tok));
	}
	else if (tok.GetType() == TT_STRING)
	{
		Advance();
		res.RegisterAdvancement();
		return res.Success(std::make_shared<StringNode>(tok));
	}
	else if (tok.GetType() == TT_IDENTIFIER)
	{
		Advance();
		res.RegisterAdvancement();
		

		std::optional<std::string> moduleAlias = std::nullopt;
		Token varNameTok = tok;

		// Handle Test::func1 pattern
		if (currentToken.GetType() == TT_DBLCOLON)
		{
			Advance();
			res.RegisterAdvancement();

			if (currentToken.GetType() != TT_IDENTIFIER)
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier after '::'"));

			moduleAlias = std::get<std::string>(varNameTok.GetValue());
			varNameTok = currentToken;

			Advance();
			res.RegisterAdvancement();
		}

		return res.Success(std::make_unique<VarAccessNode>(varNameTok, moduleAlias));
	}
	else if (tok.GetType() == TT_LPAREN)
	{
		Advance();
		res.RegisterAdvancement();
		std::shared_ptr<Node> expr = res.Register(Expr());
		if (res.HasError())
			return res;
		if (currentToken.GetType() == TT_RPAREN)
		{
			Advance();
			res.RegisterAdvancement();
			return res.Success(expr);
		}
		else
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ')'"));
	}
	else if (tok.GetType() == TT_LSQUARE)
	{
		std::shared_ptr<Node> listExpr = res.Register(ListExpr());
		if (res.HasError())
			return res;

		return res.Success(listExpr);
	}
	else if (tok.Matches(TT_KEYWORD, "IF"))
	{
		std::shared_ptr<Node> ifExpr = res.Register(IfExpr());
		if (res.HasError())
			return res;
		return res.Success(ifExpr);
	}
	else if (tok.Matches(TT_KEYWORD, "FOR"))
	{
		std::shared_ptr<Node> forExpr = res.Register(ForExpr());
		if (res.HasError())
			return res;
		return res.Success(forExpr);
	}
	else if (tok.Matches(TT_KEYWORD, "WHILE"))
	{
		std::shared_ptr<Node> whileExpr = res.Register(WhileExpr());
		if (res.HasError())
			return res;
		return res.Success(whileExpr);
	}
	else if (tok.Matches(TT_KEYWORD, "FUNC"))
	{
		std::shared_ptr<Node> funcDef = res.Register(FuncDef());
		if (res.HasError())
			return res;
		return res.Success(funcDef);
	}

	return res.Failure(std::make_unique<InvalidSyntaxError>(tok.GetPosStart(), tok.GetPosEnd(), "Expected int, float, identifier, '+', '-', '(', '[', 'IF', 'FOR', 'WHILE', 'FUNC'"));
}

ParseResult Parser::ListExpr()
{
	ParseResult res;
	std::vector<std::shared_ptr<Node>> elementNodes;
	Position posStart = currentToken.GetPosStart().Copy();

	if (currentToken.GetType() != TT_LSQUARE)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '['"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() == TT_RSQUARE)
	{
		Advance();
		res.RegisterAdvancement();
	}
	else
	{
		elementNodes.push_back(res.Register(Expr()));
		if (res.HasError())
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ']', 'VAR', 'IF', 'FOR', 'WHILE', 'FUNC', 'int', 'float', identifier, '+', '-', '(', '[' or 'NOT'"));

		while (currentToken.GetType() == TT_COMMA)
		{
			Advance();
			res.RegisterAdvancement();

			elementNodes.push_back(res.Register(Expr()));
			if (res.HasError())
				return res;
		}

		if (currentToken.GetType() != TT_RSQUARE)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ',' or ']'"));

		Advance();
		res.RegisterAdvancement();
	}

	return res.Success(std::make_unique<ListNode>(elementNodes, posStart, currentToken.GetPosEnd().Copy()));
}

ParseResult Parser::IfExpr()
{
	ParseResult res;
	CasesResult result;
	res.Register(IfExprCases("IF", result));
	if (res.HasError()) return res;

	return res.Success(std::make_shared<IfNode>(result.cases, result.elseCase));
}

ParseResult Parser::IfExprB()
{
	CasesResult dummyResult;
	return IfExprCases("ELIF", dummyResult);
}

ParseResult Parser::IfExprC(std::shared_ptr<IfCase>& outElseCase)
{
	ParseResult res;

	if (currentToken.Matches(TT_KEYWORD, "ELSE"))
	{
		Advance();
		res.RegisterAdvancement();

		if (currentToken.GetType() == TT_NEWLINE)
		{
			Advance();
			res.RegisterAdvancement();

			std::shared_ptr<Node> statements = res.Register(Statements());
			if (res.HasError())
				return res;

			outElseCase = std::make_shared<IfCase>(nullptr, statements, true);

			if (currentToken.GetType() == TT_RCURLYBRACKET)
			{
				Advance();
				res.RegisterAdvancement();
			}
			else
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '}'"));
		}
		else
		{
			std::shared_ptr<Node> expr = res.Register(Statement());
			if (res.HasError())
				return res;

			outElseCase = std::make_shared<IfCase>(nullptr, expr, false);
		}
	}
	else
	{
		outElseCase = nullptr;
	}

	return res.Success(nullptr);
}

ParseResult Parser::IfExprBorC(CasesResult& outResult)
{
	ParseResult res;

	if (currentToken.Matches(TT_KEYWORD, "ELIF"))
	{
		res.Register(IfExprCases("ELIF", outResult));
		if (res.HasError())
			return res;
	}
	else
	{
		std::shared_ptr<IfCase> elseCase;
		res.Register(IfExprC(elseCase));
		if (res.HasError())
			return res;

		if (elseCase)
			outResult.elseCase = std::make_shared<IfNode>(std::vector<IfCase>{ *elseCase }, nullptr);
		else
			outResult.elseCase = nullptr;
	}

	return res.Success(nullptr);
}

ParseResult Parser::IfExprCases(std::string caseKeyword, CasesResult& outResult)
{
	ParseResult res;
	std::vector<IfCase> cases;
	std::shared_ptr<Node> elseCase;

	if (!currentToken.Matches(TT_KEYWORD, caseKeyword))
		return res.Failure(std::make_unique<InvalidSyntaxError>(
			currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '" + caseKeyword + "'"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> condition = res.Register(Expr());
	if (res.HasError()) return res;

	if (!currentToken.Matches(TT_KEYWORD, "THEN"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(
			currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'THEN'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() == TT_NEWLINE)
	{
		Advance();
		res.RegisterAdvancement();

		std::shared_ptr<Node> body = res.Register(Statements());
		if (res.HasError()) return res;

		cases.push_back(IfCase(condition, body, true));

		if (currentToken.GetType() == TT_RCURLYBRACKET)  // End of block
		{
			Advance();
			res.RegisterAdvancement();
		}
		else
		{
			CasesResult subCases;
			res.Register(IfExprBorC(subCases));
			if (res.HasError()) return res;

			cases.insert(cases.end(), subCases.cases.begin(), subCases.cases.end());
			elseCase = subCases.elseCase;
		}
	}
	else
	{
		std::shared_ptr<Node> expr = res.Register(Statement());
		if (res.HasError()) return res;

		cases.push_back(IfCase(condition, expr, false));

		CasesResult subCases;
		res.Register(IfExprBorC(subCases));
		if (res.HasError()) return res;

		cases.insert(cases.end(), subCases.cases.begin(), subCases.cases.end());
		elseCase = subCases.elseCase;
	}

	outResult.cases = cases;
	outResult.elseCase = elseCase;
	return res.Success(nullptr);
}

ParseResult Parser::ForExpr()
{
	ParseResult res;

	if (!currentToken.Matches(TT_KEYWORD, "FOR"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'FOR'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_IDENTIFIER)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier"));

	Token varName = currentToken;

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_EQ)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '='"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> startValue = res.Register(Expr());
	if (res.HasError())
		return res;

	if (!currentToken.Matches(TT_KEYWORD, "TO"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'TO'"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> endValue = res.Register(Expr());
	if (res.HasError())
		return res;

	std::shared_ptr<Node> stepValue;
	if (currentToken.Matches(TT_KEYWORD, "STEP"))
	{
		Advance();
		res.RegisterAdvancement();
		stepValue = res.Register(Expr());
		if (res.HasError())
			return res;
	}
	else
		stepValue = nullptr;

	if (!currentToken.Matches(TT_KEYWORD, "THEN"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'THEN'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() == TT_NEWLINE)
	{
		Advance();
		res.RegisterAdvancement();

		std::shared_ptr<Node> body = res.Register(Statements());
		if (res.HasError())
			return res;

		if (currentToken.GetType() != TT_RCURLYBRACKET)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '}'"));

		Advance();
		res.RegisterAdvancement();

		return res.Success(std::make_unique<ForNode>(varName, startValue, endValue, stepValue, body, true));
	}

	std::shared_ptr<Node> body = res.Register(Statement());
	if (res.HasError())
		return res;

	return res.Success(std::make_shared<ForNode>(varName, startValue, endValue, stepValue, body, false));
}

ParseResult Parser::WhileExpr()
{
	ParseResult res;

	if (!currentToken.Matches(TT_KEYWORD, "WHILE"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'WHILE'"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> condition = res.Register(Expr());
	if (res.HasError())
		return res;

	if (!currentToken.Matches(TT_KEYWORD, "THEN"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'THEN'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() == TT_NEWLINE)
	{
		Advance();
		res.RegisterAdvancement();

		std::shared_ptr<Node> body = res.Register(Statements());
		if (res.HasError())
			return res;

		if (currentToken.GetType() != TT_RCURLYBRACKET)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '}'"));

		Advance();
		res.RegisterAdvancement();

		return res.Success(std::make_shared<WhileNode>(condition, body, true));
	}

	std::shared_ptr<Node> body = res.Register(Statement());
	if (res.HasError())
		return res;

	return res.Success(std::make_shared<WhileNode>(condition, body, false));
}

ParseResult Parser::FuncDef()
{
	ParseResult res;

	if (!currentToken.Matches(TT_KEYWORD, "FUNC"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'FUNC'"));

	Advance();
	res.RegisterAdvancement();

	std::optional<Token> varNameTok;
	if (currentToken.GetType() == TT_IDENTIFIER)
	{
		varNameTok = currentToken;

		Advance();
		res.RegisterAdvancement();

		if (currentToken.GetType() != TT_LPAREN)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '('"));
	}
	else
	{
		varNameTok = std::nullopt;

		if (currentToken.GetType() != TT_LPAREN)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier or '('"));
	}

	Advance();
	res.RegisterAdvancement();
	
	std::vector<Token> argNameToks;

	if (currentToken.GetType() == TT_IDENTIFIER)
	{
		argNameToks.push_back(currentToken);

		Advance();
		res.RegisterAdvancement();

		while (currentToken.GetType() == TT_COMMA)
		{
			Advance();
			res.RegisterAdvancement();

			if (currentToken.GetType() != TT_IDENTIFIER)
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier"));

			argNameToks.push_back(currentToken);

			Advance();
			res.RegisterAdvancement();
		}

		if (currentToken.GetType() != TT_RPAREN)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ',' or ')'"));

		Advance();
		res.RegisterAdvancement();
	}
	else if (currentToken.GetType() == TT_RPAREN)
	{
		Advance();
		res.RegisterAdvancement();
	}
	else
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier or ')'"));

	if (currentToken.GetType() == TT_ARROW)
	{
		Advance();
		res.RegisterAdvancement();

		std::shared_ptr<Node> nodeToReturn = res.Register(Expr());
		if (res.HasError())
			return res;

		return res.Success(std::make_shared<FuncDefNode>(varNameTok, argNameToks, nodeToReturn, true));
	}
	
	if (currentToken.GetType() != TT_NEWLINE)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '->' or NEWLINE"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> body = res.Register(Statements());
	if (res.HasError())
		return res;

	if (currentToken.GetType() != TT_RCURLYBRACKET)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '}'"));

	Advance();
	res.RegisterAdvancement();

	return res.Success(std::make_shared<FuncDefNode>(varNameTok, argNameToks, body, false));
}

ParseResult Parser::BinOp(std::function<ParseResult()> func_a, std::vector<std::string> ops, std::function<ParseResult()> func_b)
{
	if (func_b == nullptr)
		func_b = func_a;

	ParseResult res;
	auto left = res.Register(func_a());

	if (res.HasError())
		return res;

	while (std::find(ops.begin(), ops.end(), currentToken.GetType()) != ops.end())
	{
		Token opToken = currentToken;
		Advance();
		res.RegisterAdvancement();
		auto right = res.Register(func_b());
		if (res.HasError())
			return res;
		left = std::make_shared<BinOpNode>(left, opToken, right);
	}

	return res.Success(left);
}

ParseResult Parser::BinOp(std::function<ParseResult()> func_a, std::vector<std::pair<std::string, std::string>> typeValueOps, std::function<ParseResult()> func_b)
{
	if (func_b == nullptr)
		func_b = func_a;

	ParseResult res;
	auto left = res.Register(func_a());

	if (res.HasError())
		return res;

	while (true)
	{
		bool matched = false;
		for (auto& [type, value] : typeValueOps)
		{
			if (currentToken.GetType() == type && currentToken.Matches(type, value))
			{
				Token opToken = currentToken;
				Advance();
				res.RegisterAdvancement();

				auto right = res.Register(func_b());
				if (res.HasError())
					return res;

				left = std::make_shared<BinOpNode>(left, opToken, right);
				matched = true;
				break;
			}
		}

		if (!matched)
			break;
	}

	return res.Success(left);
}

std::shared_ptr<Node> ParseResult::Register(const ParseResult& res)
{
	advancementCount += res.advancementCount;
	if (res.error)
		this->error = std::make_unique<Error>(*res.error);

	return res.node;
}

std::optional<std::shared_ptr<Node>> ParseResult::TryRegister(const ParseResult& res)
{
	if (res.HasError())
	{
		toReverseCount = res.GetAdvancementCount();
		return std::nullopt;
	}
	return Register(res);
}

void ParseResult::RegisterAdvancement()
{
	advancementCount++;
}

ParseResult& ParseResult::Success(std::shared_ptr<Node> node)
{
	this->node = node;
	return *this;
}

ParseResult& ParseResult::Failure(std::unique_ptr<Error> error)
{
	if (error != nullptr || advancementCount == 0)
		this->error = std::move(error);
	return *this;
}
