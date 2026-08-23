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

Token Parser::Peek(int offset)
{
    if (tokIdx + offset < tokens.size())
		return tokens[tokIdx + offset];
	return currentToken; // Return current token when out-of bounds
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

		ParseResult statementResult = Statement();	
		if (statementResult.HasError())
		{
			if (statementResult.GetAdvancementCount() == 0)
			{
				// no tokens consumed at all, block just ended here
				moreStatements = false;
				continue;
			}
			
			// partially parsed before failing -> error
			res.Register(statementResult);
			return res;
		}

		statements.push_back(res.Register(statementResult));
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

	if (currentToken.Matches(TT_KEYWORD, "return"))
	{
		Advance();
		res.RegisterAdvancement();

		std::optional<std::shared_ptr<Node>> expr = res.TryRegister(Expr());
		if (!expr.has_value())
			Reverse(res.GetToReverseCount());
		return res.Success(std::make_unique<ReturnNode>(expr, posStart, currentToken.GetPosEnd().Copy()));
	}

	if (currentToken.Matches(TT_KEYWORD, "continue"))
	{
		Advance();
		res.RegisterAdvancement();

		return res.Success(std::make_unique<ContinueNode>(posStart, currentToken.GetPosEnd().Copy()));
	}

	if (currentToken.Matches(TT_KEYWORD, "break"))
	{
		Advance();
		res.RegisterAdvancement();

		return res.Success(std::make_unique<BreakNode>(posStart, currentToken.GetPosEnd().Copy()));
	}

	if (currentToken.GetType() == TT_LSQUARE)
	{
	    ParseResult attrRes = AttributeList();

	    // skip newlines between "]" and the func/struct keyword
	    while (!attrRes.HasError() && currentToken.GetType() == TT_NEWLINE)
	    {
	        Advance();
	        attrRes.RegisterAdvancement();
	    }

	    if (!attrRes.HasError() && (currentToken.Matches(TT_KEYWORD, "func") || currentToken.Matches(TT_KEYWORD, "struct")))
	    {
	        std::shared_ptr<Node> def = res.Register(currentToken.Matches(TT_KEYWORD, "func") ? FuncDef() : StructDef());
	        if (res.HasError())
	            return res;

	        if (auto* func = dynamic_cast<FuncDefNode*>(def.get()))
	            func->SetAttributes(m_pendingAttributes);

	        // ToDo: Extend struct def to also take in attributes

	        m_pendingAttributes.clear();
	        return res.Success(def);
	    }

	    // wasn't attributes, reverse everything and fall through
	    Reverse(attrRes.GetAdvancementCount());
	}

	if (currentToken.GetType() == TT_HASH)
	{
		std::shared_ptr<Node> importStatement = res.Register(ModuleExternLinkStatement());
		if (res.HasError())
			return res;

		return res.Success(importStatement);
	}

	std::shared_ptr<Node> expr = res.Register(Expr());
	if (res.HasError())
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected int, float, identifier, var, '+', '-', '(', '[', 'if', 'for', 'while', 'func', 'return', 'continue', 'break' or not"));

	return res.Success(expr);
}

ParseResult Parser::ModuleExternLinkStatement()
{
	ParseResult res;
	Position posStart = currentToken.GetPosStart().Copy();
	bool link = false;

	Advance();
	res.RegisterAdvancement();

	if (currentToken.Matches(TT_KEYWORD, "module"))
		link = false;
	else if (currentToken.Matches(TT_KEYWORD, "link"))
		link = true;
	else if (currentToken.Matches(TT_KEYWORD, "extern"))
	{
		std::shared_ptr<Node> externNode = res.Register(ExternStatement());
		if (res.HasError()) return res;
		return res.Success(externNode);
	}
	else
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'module', 'link' or 'extern'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_STRING)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected string for file path"));

	Token filepathToken = currentToken;

	Advance();
	res.RegisterAdvancement();

	if (!currentToken.Matches(TT_KEYWORD, "as"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'as'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_IDENTIFIER)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier as alias"));

	std::string alias = std::get<std::string>(currentToken.GetValue());

	Advance();
	res.RegisterAdvancement();

	if (!link)
		return res.Success(std::make_unique<ModuleNode>(filepathToken, alias, posStart, currentToken.GetPosEnd().Copy()));
	else
		return res.Success(std::make_unique<LinkNode>(filepathToken, alias, posStart, currentToken.GetPosEnd().Copy()));
}

ParseResult Parser::ExternStatement()
{
	ParseResult res;
	Position posStart = currentToken.GetPosStart().Copy();

	if (!currentToken.Matches(TT_KEYWORD, "extern"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'extern'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_IDENTIFIER)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected module name identifier"));

	std::string moduleAlias = std::get<std::string>(currentToken.GetValue());

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_DBLCOLON)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '::' after module name"));

	Advance(),
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_IDENTIFIER)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected function name identifier after '::'"));

	std::string functionName = std::get<std::string>(currentToken.GetValue());

	Advance();
	res.RegisterAdvancement();

	if (!currentToken.Matches(TT_KEYWORD, "as"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'as'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_STRING)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected signature string like \"int(int, int)\""));

	std::string signature = std::get<std::string>(currentToken.GetValue());

	Advance();
	res.RegisterAdvancement();

	return res.Success(std::make_unique<ExternNode>(moduleAlias, functionName, signature, posStart, currentToken.GetPosEnd().Copy()));
}

ParseResult Parser::Expr()
{
	ParseResult res;

	// Handle var declaration
	if (currentToken.GetType() == TT_IDENTIFIER && Peek().GetType() == TT_COLON)
	{
		Token varName = currentToken;

		Advance();
		res.RegisterAdvancement();

		if (currentToken.GetType() != TT_COLON)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ':'"));

		Advance();
		res.RegisterAdvancement();

		if (currentToken.Matches(TT_KEYWORD, "var"))
		{
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
				return res.Success(std::make_shared<VarAssignNode>(varName, expr, true, std::nullopt));
		}

		// check if var type is validly typed
		std::string strictVarType = "";
		if (std::holds_alternative<std::string>(currentToken.GetValue()))
		{
			strictVarType = std::get<std::string>(currentToken.GetValue());
			if (strictVarType == "")
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Variable datatype can not be nothing"));
		}
		else
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Could not parse var type. Type was not a string"));

		Advance();
		res.RegisterAdvancement();

		// check if datatype is a list
		if (currentToken.GetType() == TT_LSQUARE)
		{
			Advance();
			res.RegisterAdvancement();

			bool dynamicSize = false;
			int fixedSize = 0;
			if (currentToken.Matches(TT_KEYWORD, "dyn"))
				dynamicSize = true;
			else if (currentToken.GetType() == TT_INT)
				fixedSize = std::get<int>(currentToken.GetValue());
			else
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "List size must be a <number> as integer or 'dyn'"));

			Advance();
			res.RegisterAdvancement();

			if (currentToken.GetType() != TT_RSQUARE)
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ']'"));

			strictVarType += "[" + (dynamicSize ? std::string("dyn") : std::to_string(fixedSize)) + "]";

			Advance();
			res.RegisterAdvancement();
		}
		
		if (currentToken.GetType() == TT_EQ)
		{
			Advance();
			res.RegisterAdvancement();
			auto expr = res.Register(Expr());

			if (res.HasError())
				return res;
			else
				return res.Success(std::make_shared<VarAssignNode>(varName, expr, true, std::nullopt, strictVarType));
		}
		else // no direct assignment
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Variables must be initialized"));
	}

	// Handle indexed assignment
	if (currentToken.GetType() == TT_IDENTIFIER && Peek().GetType() == TT_LSQUARE)
	{
		std::optional<std::shared_ptr<Node>> indexAssign = res.TryRegister(IndexAssignStatement());
		if (indexAssign.has_value())
			return res.Success(indexAssign.value());
		else
			Reverse(res.GetToReverseCount());
	}

	// Handle assignment
	if (currentToken.GetType() == TT_IDENTIFIER && Peek().GetType() == TT_EQ)
	{
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
			return res.Success(std::make_shared<VarAssignNode>(varName, expr, false, std::nullopt));
	}

	std::vector<std::pair<std::string, std::string>> ops = {
	{ TT_KEYWORD, "and"},
	{ TT_KEYWORD, "or"}
	};
	std::shared_ptr<Node> node = res.Register(BinOp([this]() {return CompExpr(); }, ops));

	if (res.HasError())
	{
		if (res.GetAdvancementCount() == 0 && !res.GetErrorPtr())
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(),"Expected int, float, identifier, var, '+', '-', '(', '[', 'if', 'for', 'while', 'func' or not"));
		return res;
	}

	return res.Success(node);
}

ParseResult Parser::CompExpr()
{
	ParseResult res;

	if (currentToken.Matches(TT_KEYWORD, "not"))
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
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected int, float, identifier, var, '+', '-', '(', '[' or 'not'"));
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
	std::vector<std::string> ops = { TT_MUL, TT_DIV, TT_MOD };
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

	// postfix indexing: arr[i], and chained arr[i][j]
	while (currentToken.GetType() == TT_LSQUARE)
	{
		Position indexPosStart = atom->GetPosStart();
		
		Advance();
		res.RegisterAdvancement();

		std::shared_ptr<Node> indexExpr = res.Register(Expr());
		if (res.HasError())
			return res;

		if (currentToken.GetType() != TT_RSQUARE)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ']'"));

		Advance();
		res.RegisterAdvancement();

		atom = std::make_shared<IndexGetNode>(atom, indexExpr, indexPosStart, currentToken.GetPosEnd().Copy());
	}

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
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ')', 'var', 'if', 'for', 'while', 'func', 'int', 'float', identifier, '+', '-', '(', '[' or 'not'"));

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
		
		Token varNameTok = tok;

		// Handle Test::func1 and Test::varA = 100 pattern
		if (currentToken.GetType() == TT_DBLCOLON)
		{
			Advance();
			res.RegisterAdvancement();

			if (currentToken.GetType() != TT_IDENTIFIER)
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier after '::'"));

			std::optional<std::string> namespaceName = std::get<std::string>(varNameTok.GetValue());
			varNameTok = currentToken;

			Advance();
			res.RegisterAdvancement();

			if (currentToken.GetType() != TT_EQ)
				return res.Success(std::make_unique<VarAccessNode>(varNameTok, namespaceName));
			else // Handle var assignment through namespace
			{
				Advance();
				res.RegisterAdvancement();

				auto expr = res.Register(Expr());

				if (res.HasError())
					return res;
				else
					return res.Success(std::make_shared<VarAssignNode>(varNameTok, expr, false, namespaceName));
			}
		}

		return res.Success(std::make_unique<VarAccessNode>(varNameTok, std::nullopt));
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
	else if (tok.Matches(TT_KEYWORD, "if"))
	{
		std::shared_ptr<Node> ifExpr = res.Register(IfExpr());
		if (res.HasError())
			return res;
		return res.Success(ifExpr);
	}
	else if (tok.Matches(TT_KEYWORD, "for"))
	{
		std::shared_ptr<Node> forExpr = res.Register(ForExpr());
		if (res.HasError())
			return res;
		return res.Success(forExpr);
	}
	else if (tok.Matches(TT_KEYWORD, "while"))
	{
		std::shared_ptr<Node> whileExpr = res.Register(WhileExpr());
		if (res.HasError())
			return res;
		return res.Success(whileExpr);
	}
	else if (tok.Matches(TT_KEYWORD, "func"))
	{
		std::shared_ptr<Node> funcDef = res.Register(FuncDef());
		if (res.HasError())
			return res;
		return res.Success(funcDef);
	}
	else if (tok.Matches(TT_KEYWORD, "struct"))
	{
		std::shared_ptr<Node> structDef = res.Register(StructDef());
		if (res.HasError())
			return res;
		return res.Success(structDef);
	}

	return res.Failure(std::make_unique<InvalidSyntaxError>(tok.GetPosStart(), tok.GetPosEnd(), "Expected int, float, identifier, '+', '-', '(', '[', 'if', 'for', 'while', 'func' or 'struct'"));
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
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ']', 'var', 'if', 'for', 'while', 'func', 'int', 'float', identifier, '+', '-', '(', '[' or 'not'"));

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

ParseResult Parser::IndexAssignStatement()
{
    ParseResult res;
	Position posStart = currentToken.GetPosStart().Copy();

	if (currentToken.GetType() != TT_IDENTIFIER)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier"));

	Token varName = currentToken;

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_LSQUARE)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '['"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> indexExpr = res.Register(Expr());
	if (res.HasError())
		return res;

	if (currentToken.GetType() != TT_RSQUARE)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ']'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_EQ)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '='"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> valueExpr = res.Register(Expr());
	if (res.HasError())
		return res;

	std::shared_ptr<Node> listNode = std::make_shared<VarAccessNode>(varName, std::nullopt);
	return res.Success(std::make_shared<IndexAssignNode>(listNode, indexExpr, valueExpr, posStart, currentToken.GetPosEnd().Copy()));
}

ParseResult Parser::IfExpr()
{
	ParseResult res;
	CasesResult result;
	res.Register(IfExprCases("if", result));
	if (res.HasError()) return res;

	return res.Success(std::make_shared<IfNode>(result.cases, result.elseCase));
}

ParseResult Parser::IfExprB()
{
	CasesResult dummyResult;
	return IfExprCases("elif", dummyResult);
}

ParseResult Parser::IfExprC(std::shared_ptr<IfCase>& outElseCase)
{
	ParseResult res;

	if (currentToken.Matches(TT_KEYWORD, "else"))
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

	if (currentToken.Matches(TT_KEYWORD, "elif"))
	{
		res.Register(IfExprCases("elif", outResult));
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

	if (!currentToken.Matches(TT_KEYWORD, "then"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(
			currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'then'"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() == TT_NEWLINE)
	{
		Advance();
		res.RegisterAdvancement();

		// Check for body
		bool hasBodyContent = true;
		std::shared_ptr<Node> body = nullptr;
		while (currentToken.GetType() == TT_NEWLINE)
		{
			Advance();
			res.RegisterAdvancement();
		}
		if (currentToken.GetType() == TT_RCURLYBRACKET)
		{
			hasBodyContent = false;
			std::cout << "Warning(l." << currentToken.GetPosStart().GetLineNumber() << "): If statement has empty body\n";
		}

		if (hasBodyContent)
		{
			body = res.Register(Statements());
			if (res.HasError()) return res;
		}

		cases.push_back(IfCase(condition, body, true));

		if (currentToken.GetType() != TT_RCURLYBRACKET)  // End of block
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '}'"));

		Advance();
		res.RegisterAdvancement();

		if (currentToken.GetType() == TT_NEWLINE && (Peek().Matches(TT_KEYWORD, "elif") || Peek().Matches(TT_KEYWORD, "else")))
		{
			Advance();
			res.RegisterAdvancement();
		}

		if (currentToken.Matches(TT_KEYWORD, "elif") || currentToken.Matches(TT_KEYWORD, "else"))
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

		// Only continue chain if next token id elif or else	
		if (currentToken.Matches(TT_KEYWORD, "elif") || currentToken.Matches(TT_KEYWORD, "else"))
		{
			CasesResult subCases;
			res.Register(IfExprBorC(subCases));
			if (res.HasError()) return res;

			cases.insert(cases.end(), subCases.cases.begin(), subCases.cases.end());
			elseCase = subCases.elseCase;
		}
	}

	outResult.cases.insert(outResult.cases.end(), cases.begin(), cases.end());
	outResult.elseCase = elseCase;
	return res.Success(nullptr);
}

ParseResult Parser::ForExpr()
{
	ParseResult res;

	if (!currentToken.Matches(TT_KEYWORD, "for"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'for'"));

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

	if (!currentToken.Matches(TT_KEYWORD, "to"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'to'"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> endValue = res.Register(Expr());
	if (res.HasError())
		return res;

	std::shared_ptr<Node> stepValue;
	if (currentToken.Matches(TT_KEYWORD, "step"))
	{
		Advance();
		res.RegisterAdvancement();
		stepValue = res.Register(Expr());
		if (res.HasError())
			return res;
	}
	else
		stepValue = nullptr;

	if (!currentToken.Matches(TT_KEYWORD, "then"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'then'"));

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

	if (!currentToken.Matches(TT_KEYWORD, "while"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'while'"));

	Advance();
	res.RegisterAdvancement();

	std::shared_ptr<Node> condition = res.Register(Expr());
	if (res.HasError())
		return res;

	if (!currentToken.Matches(TT_KEYWORD, "then"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'then'"));

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

	if (!currentToken.Matches(TT_KEYWORD, "func"))
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'func'"));

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
	
	bool argByReference = false;
	std::vector<ArgNameToken> argNameToks;

	if (currentToken.Matches(TT_KEYWORD, "ref"))
	{
		argByReference = true;

		Advance();
		res.RegisterAdvancement();
	}

	if (currentToken.GetType() == TT_IDENTIFIER)
	{
		argNameToks.push_back(ArgNameToken(currentToken, argByReference));

		Advance();
		res.RegisterAdvancement();

		while (currentToken.GetType() == TT_COMMA)
		{
			Advance();
			res.RegisterAdvancement();

			argByReference = false;

			if (currentToken.Matches(TT_KEYWORD, "ref"))
			{
				argByReference = true;

				Advance();
				res.RegisterAdvancement();
			}

			if (currentToken.GetType() != TT_IDENTIFIER)
				return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier"));

			argNameToks.push_back(ArgNameToken(currentToken, argByReference));

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

	std::optional<std::shared_ptr<Node>> body = std::nullopt;
	body = res.TryRegister(Statements());
	if (body.has_value() && res.HasError())
		return res;

	if (currentToken.GetType() != TT_RCURLYBRACKET)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '}'"));

	Advance();
	res.RegisterAdvancement();

	return res.Success(std::make_shared<FuncDefNode>(varNameTok, argNameToks, body.has_value() ? body.value() : nullptr, false));
}

ParseResult Parser::StructDef()
{
	ParseResult res;

	if (!currentToken.Matches(TT_KEYWORD, "struct"))
		res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected 'struct'"));

	Advance();
	res.RegisterAdvancement();

	Token varNameTok;
	if (currentToken.GetType() == TT_IDENTIFIER)
		varNameTok = currentToken;
	else
		res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier"));

	Advance();
	res.RegisterAdvancement();

	if (currentToken.GetType() != TT_NEWLINE)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected NEWLINE"));

	Advance();
	res.RegisterAdvancement();

	std::vector<StructAttributeToken> attributeToks;

	while (currentToken.GetType() == TT_IDENTIFIER)
	{
		std::string attributeType = "";
		std::string attributeName = "";

		if (currentToken.GetType() != TT_IDENTIFIER)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected type as identifier"));
		else
			attributeType = std::get<std::string>(currentToken.GetValue());

		Advance();
		res.RegisterAdvancement();

		if (currentToken.GetType() != TT_IDENTIFIER)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected identifier"));
		else
			attributeName = std::get<std::string>(currentToken.GetValue());

		Advance();
		res.RegisterAdvancement();

		if (currentToken.GetType() != TT_NEWLINE)
			return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected NEWLINE"));

		Advance();
		res.RegisterAdvancement();

		attributeToks.push_back(StructAttributeToken(attributeType, attributeName));
	}

	if (currentToken.GetType() != TT_RCURLYBRACKET)
		return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected '}'"));

	Advance();
	res.RegisterAdvancement();

	return res.Success(std::make_shared<StructDefNode>(varNameTok, attributeToks));
}

ParseResult Parser::AttributeList()
{
    ParseResult res;
    std::vector<FuncAttribute> attrs;

    Advance();
    res.RegisterAdvancement();

    auto parseOne = [&]() -> bool
    {
        if (currentToken.GetType() != TT_IDENTIFIER)
            return false;

        std::string attrName = std::get<std::string>(currentToken.GetValue());
        Advance();
        res.RegisterAdvancement();

        std::optional<std::string> arg;
        if (currentToken.GetType() == TT_LPAREN)
        {
            Advance();
            res.RegisterAdvancement();

            if (currentToken.GetType() != TT_STRING)
                return false;
            arg = std::get<std::string>(currentToken.GetValue());
            Advance();
            res.RegisterAdvancement();

            if (currentToken.GetType() != TT_RPAREN)
                return false;
            Advance();
            res.RegisterAdvancement();
        }

        attrs.push_back({ attrName, arg });
        return true;
    };

    if (!parseOne())
        return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected attribute name"));

    while (currentToken.GetType() == TT_COMMA)
    {
        Advance();
        res.RegisterAdvancement();
        if (!parseOne())
            return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected attribute name"));
    }

    if (currentToken.GetType() != TT_RSQUARE)
        return res.Failure(std::make_unique<InvalidSyntaxError>(currentToken.GetPosStart(), currentToken.GetPosEnd(), "Expected ']'"));
    Advance();
    res.RegisterAdvancement();

    m_pendingAttributes = attrs;
    return res.Success(nullptr);
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
	if (!this->error || advancementCount == 0)
		this->error = std::move(error);
	return *this;
}
