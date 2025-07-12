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
    globalSymbolTable.Set("MATH_PI", static_cast<double>(3.141592653589793));
    globalSymbolTable.Set("PRINT", std::make_shared<NativePrintFunction>());
    globalSymbolTable.Set("LENGTH", std::make_shared<NativeLengthFunction>());
    globalSymbolTable.Set("INPUT_STR", std::make_shared<NativeInputStr>());
    globalSymbolTable.Set("INPUT_NUM", std::make_shared<NativeInputNum>());
    globalSymbolTable.Set("IS_NUM", std::make_shared<NativeIsNum>());
    globalSymbolTable.Set("IS_STR", std::make_shared<NativeIsStr>());
    globalSymbolTable.Set("IS_LIST", std::make_shared<NativeIsList>());
    globalSymbolTable.Set("IS_FUNC", std::make_shared<NativeIsFunc>());
    globalSymbolTable.Set("APPEND", std::make_shared<NativeAppend>());
    globalSymbolTable.Set("POP", std::make_shared<NativePop>());
    globalSymbolTable.Set("EXTEND", std::make_shared<NativeExtend>());
    globalSymbolTable.Set("CLEAR", std::make_shared<NativeClear>());

    while (true)
    {
        std::string text;
        std::cout << "E++ > ";
        std::getline(std::cin, text);

        // Check for empty input with or without whitespace characters
        std::string trimText = text;
        trimText.erase(std::remove_if(trimText.begin(), trimText.end(), [](unsigned char c) { return std::isspace(c); }), trimText.end());
        if (trimText.empty())
            continue;

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
            std::cout << Helper::Print(result) << std::endl;
        }
    }
}