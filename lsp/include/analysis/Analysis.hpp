#pragma once
#include "State.hpp"
#include "../lsp/TextDocument.hpp"
#include "../../../core/include/Lexer.hpp"
#include "../../../core/include/Parser.hpp"
#include "../rpc/rpc.hpp"

void PublishClearError(const std::string& uri)
{
    auto msg = NewPublishDiagnosticsClearResponse(uri);
    auto reply = EncodeMessage(msg);

    std::fwrite(reply.c_str(), 1, reply.size(), stdout);
    std::fflush(stdout);

    LOG("Send PublishDiagnostics notification to clear errors");
}

void PublishError(const std::string& uri, Error* const & error, const std::string errorSource)
{
    // Clear old error
    PublishClearError(uri);

    // Send new error
    auto msg = NewPublishDiagnosticsErrorResponse(uri, error, errorSource);
    auto reply = EncodeMessage(msg);

    std::fwrite(reply.c_str(), 1, reply.size(), stdout);
    std::fflush(stdout);

    LOG("Send PublishDiagnostics notification with made errors");
}

void AnalyseForErrors(State* state, std::string uri)
{
    // Generate tokens
    Lexer lexer(uri, state->documents[uri]);
    MakeTokensResult tokenResult = lexer.MakeTokens();
    if (tokenResult.error != nullptr)
    {
        PublishError(uri, tokenResult.error.get(), "Eugenpp Lexer");

        LOG("Got lexing error: " + tokenResult.error.get()->GetErrorDetails());
        return;
    }

    // Generate AST
    Parser parser(tokenResult.tokens);
    ParseResult ast = parser.Parse();
    if (ast.HasError())
    {
        PublishError(uri, ast.GetErrorPtr(), "Eugenpp Parser");

        LOG("Got parsing error: " + ast.GetErrorPtr()->GetErrorDetails());
        return;
    }

    PublishClearError(uri);
}