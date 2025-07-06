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
	if (tokIdx < tokens.size())
		currentToken = tokens[tokIdx];

	return currentToken;
}

ParseResult Parser::Parse()
{
	//std::cout << "Parse!" << std::endl;
	ParseResult res = Expr();
	if (!res.HasError() && currentToken.GetType() != TT_EOF)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '+', '-', '*' or '/'"));
	return res;
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
		return res.Success(std::make_shared<VarAccessNode>(tok));
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
	std::vector<IfCase> cases;
	std::shared_ptr<Node> elseCase;

	if (!currentToken.Matches(TT_KEYWORD, "IF"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'IF'"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> condition = res.Register(Expr());
	if (res.HasError())
		return res;

	if (!currentToken.Matches(TT_KEYWORD, "THEN"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'THEN'"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> expr = res.Register(Expr());
	if (res.HasError())
		return res;

	cases.push_back(IfCase(condition, expr));

	while (currentToken.Matches(TT_KEYWORD, "ELIF"))
	{
		Advance();
		res.RegisterAdvancement();

		condition = res.Register(Expr());
		if (res.HasError())
			return res;

		if (!currentToken.Matches(TT_KEYWORD, "THEN"))
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'THEN'"));

		Advance();
		res.RegisterAdvancement();

		expr = res.Register(Expr());
		if (res.HasError())
			return res;

		cases.push_back(IfCase(condition, expr));
	}

	if (currentToken.Matches(TT_KEYWORD, "ELSE"))
	{
		Advance();
		res.RegisterAdvancement();

		elseCase = res.Register(Expr());
		if (res.HasError())
			return res;
	}

	return res.Success(std::make_shared<IfNode>(cases, elseCase));
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

	std::shared_ptr<Node> body = res.Register(Expr());
	if (res.HasError())
		return res;

	return res.Success(std::make_shared<ForNode>(varName, startValue, endValue, stepValue, body));
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

	std::shared_ptr<Node> body = res.Register(Expr());
	if (res.HasError())
		return res;

	return res.Success(std::make_shared<WhileNode>(condition, body));
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
	else
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier or '('"));

	if (currentToken.GetType() != TT_ARROW)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '->'"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> nodeToReturn = res.Register(Expr());
	if (res.HasError())
		return res;

	return res.Success(std::make_shared<FuncDefNode>(varNameTok, argNameToks, nodeToReturn));
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
