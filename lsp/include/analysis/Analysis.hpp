#pragma once
#include "State.hpp"
#include "../lsp/TextDocument.hpp"
#include "../../../core/include/Lexer.hpp"
#include "../../../core/include/Parser.hpp"
#include "../rpc/rpc.hpp"

void PublishClearErrorAndWarnings(const std::string& uri)
{
    auto msg = NewPublishDiagnosticsClearResponse(uri);
    auto reply = EncodeMessage(msg);

    std::fwrite(reply.c_str(), 1, reply.size(), stdout);
    std::fflush(stdout);

    LOG("Send PublishDiagnostics notification to clear errors");
}

void PublishErrorAndWarnings(const std::string& uri, Error* const & error, const std::vector<Warning>& warnings, const std::string originSource)
{
    // Clear old error
    PublishClearErrorAndWarnings(uri);

    // Send new error
    auto msg = NewPublishDiagnosticsResponse(uri, error, warnings, originSource);
    auto reply = EncodeMessage(msg);

    std::fwrite(reply.c_str(), 1, reply.size(), stdout);
    std::fflush(stdout);

    LOG("Send PublishDiagnostics notification with made errors");
}

void Analyse(State* state, std::string uri)
{
    // Generate tokens
    Lexer lexer(uri, state->documents[uri]);
    MakeTokensResult tokenResult = lexer.MakeTokens();
    if (tokenResult.error != nullptr)
    {
        PublishErrorAndWarnings(uri, tokenResult.error.get(), {}, "Eugenpp Lexer");

        LOG("Got lexing error: " + tokenResult.error.get()->GetErrorDetails());
        return;
    }

    // Generate AST
    Parser parser(tokenResult.tokens);
    ParseResult ast = parser.Parse();
    if (ast.HasError() || parser.GetWarnings().size() > 0)
    {
        PublishErrorAndWarnings(uri, ast.GetErrorPtr(), parser.GetWarnings(), "Eugenpp Parser");

        if (ast.HasError())
            LOG("Got parsing error: " + ast.GetErrorPtr()->GetErrorDetails());
        return;
    }

    PublishClearErrorAndWarnings(uri);
}