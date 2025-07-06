#include <iostream>
#include "Lexer.hpp"
#include "Helper.hpp"
#include <string>
#include "Parser.hpp"
#include "Interpreter.hpp"

std::string print(RTResult result)
{
    if (result.GetValue().has_value())
    {
        auto val = result.GetValue().value();
        if (std::holds_alternative<double>(val))                        // Print number
        {
            double number = std::get<double>(val);
            if (number == static_cast<int>(number))                         // Print number as int
                return std::to_string(static_cast<int>(number));
            else                                                            // Print number as double
                return std::to_string(std::get<double>(val));
        }
        else if (std::holds_alternative<std::string>(val))              // Print string
            return (std::get<std::string>(val));
        else if (std::holds_alternative<List>(val))                     // Print list
        {
            const auto& list = std::get<List>(val);
            std::string result = "[";
            for (size_t i = 0; i < list.elements.size(); ++i)
            {
                result += print(RTResult().Success(list.elements[i]));
                if (i != list.elements.size() - 1)
                {
                    result += ", ";
                }
            }
            result += "]";
            return result;
        }
    }
    else
        return "";
}

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
            std::cout << print(result) << std::endl;
        }
    }
}