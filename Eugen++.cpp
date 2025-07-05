#include <iostream>
#include "Lexer.hpp"
#include "Helper.hpp"
#include <string>
#include "Parser.hpp"
#include "Interpreter.hpp"

int main()
{
    SymbolTable globalSymbolTable = SymbolTable();
    globalSymbolTable.Set("NULL", static_cast<double>(0));
    globalSymbolTable.Set("TRUE", static_cast<double>(1));
    globalSymbolTable.Set("FALSE", static_cast<double>(0));

    while (true)
    {
        std::string text;
        std::cout << "E++ > ";
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
            auto val = result.GetValue().value();
            if (std::holds_alternative<double>(val))
                std::cout << std::get<double>(val) << std::endl;
            else if (std::holds_alternative<std::string>(val))
                std::cout << std::get<std::string>(val) << std::endl;
        }
    }
}