#include <iostream>
#include "Lexer.hpp"
#include "Helper.hpp"
#include <string>
#include "Parser.hpp"
#include "Interpreter.hpp"

int main()
{
    SymbolTable globalSymbolTable = SymbolTable();
    globalSymbolTable.Set("null", 0);

    while (true)
    {
        std::string text;
        std::cout << "basic > ";
        std::getline(std::cin, text);

        // Generate tokens
        Lexer lexer("<stdin>", text);
        MakeTokensResult tokenResult = lexer.MakeTokens();

        if (tokenResult.error != nullptr)
        {
            std::cout << tokenResult.error->AsString() << std::endl;
            continue;
        }

        //Print tokenResult
        //std::cout << "Token result: " << Helper::TokenVectorToString(tokenResult.tokens) << std::endl;

        // Generate AST
        Parser parser(tokenResult.tokens);
        ParseResult ast = parser.Parse();

        if (ast.HasError())
        {
            std::cout << ast.GetError() << std::endl;
            continue;
        }
            
        //Print ast
        //std::cout << "AST: " << ast.GetNode()->Repr() << std::endl;

        Interpreter interpreter(globalSymbolTable);
        auto result = interpreter.Visit(ast.GetNode());

        if (result.HasError())
        {
            std::cout << result.GetError() << std::endl;
            continue;
        }

        if (result.GetValue().has_value())
        {
            std::cout << result.GetValue().value() << std::endl;
        }
    }
}