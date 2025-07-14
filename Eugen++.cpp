#include <iostream>
#include "Lexer.hpp"
#include "Helper.hpp"
#include <string>
#include "Parser.hpp"
#include "Interpreter.hpp"
#include <fstream>
#include <filesystem>

int main(int argc, char** argv)
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
    globalSymbolTable.Set("SYSTEM", std::make_shared<NativeSystem>());

    std::string text;
    bool loadedFromFile = false;
    if (argc > 1)
    {
        std::filesystem::path filepath = argv[1];

        // Check if the file exists and is a regular file
        if (std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath))
        {
            std::ifstream file(filepath);
            if (!file)
            {
                std::cout << "Failed to open the file.\n";
                return 1;
            }

            // Read file content into a string
            std::string content((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());

            text = content;
            loadedFromFile = true;
        }
    }

    while (true)
    {
        if (!loadedFromFile)
        {
            std::cout << "E++ > ";
            std::getline(std::cin, text);
        }

        // Check for empty input with or without whitespace characters
        std::string trimText = text;
        trimText.erase(std::remove_if(trimText.begin(), trimText.end(), [](unsigned char c) { return std::isspace(c); }), trimText.end());
        if (trimText.empty())
            continue;

        // Generate tokens
        Lexer lexer(loadedFromFile ? std::filesystem::path(argv[1]).filename().string() : "<stdin>", text);
        MakeTokensResult tokenResult = lexer.MakeTokens();

        if (tokenResult.error != nullptr)
        {
            std::cout << tokenResult.error->AsString() << std::endl;

            if (!loadedFromFile)
                continue;
            else
                break;
        }

        // Print tokenResult
        if (Helper::argv_has(argc, argv, "--tokens"))
            std::cout << "Token result: " << Helper::TokenVectorToString(tokenResult.tokens) << std::endl;

        // Generate AST
        Parser parser(tokenResult.tokens);
        ParseResult ast = parser.Parse();

        if (ast.HasError())
        {
            std::cout << ast.GetError() << std::endl;

            if (!loadedFromFile)
                continue;
            else
                break;
        }
            
        // Print ast
        if (Helper::argv_has(argc, argv, "--ast"))
            std::cout << "AST: " << ast.GetNode()->Repr() << std::endl;

        Interpreter interpreter(globalSymbolTable);
        interpreter.SetMainFilePath(loadedFromFile ? argv[1] : std::filesystem::current_path().string());
        auto result = interpreter.Visit(ast.GetNode());

        if (result.HasError())
        {
            std::cout << result.GetError() << std::endl;

            if (!loadedFromFile)
                continue;
            else
                break;
        }

        // Print result
        if (result.GetValue().has_value())
        {
            if (!loadedFromFile)
                std::cout << Helper::Print(result) << std::endl;
            else // Because when reading from file everything is inputted as on giant block, so every result in it is in another list
            {
                if (std::holds_alternative<std::shared_ptr<List>>(result.GetValue().value()))
                {
                    auto listPtr = std::get<std::shared_ptr<List>>(result.GetValue().value());
                    for (const auto& element : listPtr->elements)
                    {
                        std::cout << Helper::Print(RTResult().Success(element)) << std::endl;
                    }
                }
                else
                {
                    std::cout << Helper::Print(result) << std::endl;
                }
            }
        }

        if (loadedFromFile)
            break;
    }
}