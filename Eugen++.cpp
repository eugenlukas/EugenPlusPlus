#include <iostream>
#include "Lexer.hpp"
#include "Helper.hpp"
#include <string>
#include "Parser.hpp"

int main()
{
    while (true)
    {
        std::string text;
        std::cout << "basic > ";
        std::getline(std::cin, text);

        // Generate tokens
        Lexer lexer("<stdin>", text);
        MakeTokensResult result = lexer.MakeTokens();

        if (result.error != nullptr)
        {
            std::cout << result.error->AsString() << std::endl;
            continue;
        }

        // Generate AST
        Parser parser(result.tokens);
        ParseResult ast = parser.Parse();

        if (ast.HasError())
            std::cout << ast.GetError() << std::endl;
        else
            std::cout << ast.GetNode()->Repr() << std::endl;
    }
}