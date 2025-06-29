#include <iostream>
#include "Lexer.hpp"
#include "Helper.hpp"
#include <string>

int main()
{
    std::string text;
    std::cout << "basic > ";
    std::getline(std::cin, text);

    Lexer lexer("<stdin>", text);
    MakeTokensResult result = lexer.MakeTokens();

    if (result.error != nullptr)
        std::cout << result.error->AsString() << std::endl;
    else
        std::cout << Helper::TokenVectorToString(result.tokens) << std::endl;
}