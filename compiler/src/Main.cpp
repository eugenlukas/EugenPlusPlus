#include <iostream>
#include "Lexer.hpp"
#include "Helper.hpp"
#include <string>
#include "Parser.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <Compiler.hpp>

int main(int argc, char** argv)
{
    std::string text;
    if (argc < 2)
    {
        std::cout << "No file to compile was given!\n";
        return 1;
    }

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
    }

    // Check for empty input with or without whitespace characters
    std::string trimText = text;
    trimText.erase(std::remove_if(trimText.begin(), trimText.end(), [](unsigned char c) { return std::isspace(c); }), trimText.end());
    if (trimText.empty())
        return 0;

    // Generate tokens
    Lexer lexer(std::filesystem::path(argv[1]).filename().string(), text);
    MakeTokensResult tokenResult = lexer.MakeTokens();
    if (tokenResult.error != nullptr)
    {
        std::cerr << Helper::GetErrorString(tokenResult.error.get()) << std::endl;
        return 1;
    }

    // Print tokenResult when wished
    if (Helper::argv_has(argc, argv, "--tokens"))
        std::cout << "Token result: " << Helper::TokenVectorToString(tokenResult.tokens) << std::endl;

    // Generate AST
    Parser parser(tokenResult.tokens);
    ParseResult ast = parser.Parse();
    if (ast.HasError())
    {
        std::cerr << Helper::GetErrorString(ast.GetErrorPtr()) << std::endl;
        return 1;
    }
        
    // Print ast when wished
    if (Helper::argv_has(argc, argv, "--ast"))
        std::cout << "AST:\n" << ast.GetNode()->Repr() << std::endl;

    // Compile
    auto filepathNoEndfile(filepath);
    auto absoluteFilepathNoEndfile = std::filesystem::canonical(filepathNoEndfile);
    absoluteFilepathNoEndfile.remove_filename();
    bool dumpIR = Helper::argv_has(argc, argv, "--dumpIR");

    Compiler compiler;
    compiler.GenerateIR(ast.GetNode(), dumpIR);
    compiler.EmitObjectFile("output.o");
    if(!Helper::argv_has(argc, argv, "--o"))
        compiler.LinkObjectFile(absoluteFilepathNoEndfile);
}