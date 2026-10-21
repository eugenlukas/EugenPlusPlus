#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <vector>
#include "Message.hpp"
#include "../../../core/include/Error.hpp"
#include "../../../core/include/Warning.hpp"

#define TextDocumentIdentifierMacro \
    std::string uri;

struct TextDocumentIdentifier
{
    std::string uri;
};

struct TextDocumentItem
{
    std::string uri;
    std::string languageId;
    int version;
    std::string text;
};

struct VersionTextDocumentIdentifier
{
    TextDocumentIdentifierMacro
    int version;
};

struct LSPPosition
{
    int line;
    int character;
};

struct Range
{
    LSPPosition start;
    LSPPosition end;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextDocumentIdentifier, uri)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextDocumentItem, uri, languageId, version, text)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VersionTextDocumentIdentifier, uri, version)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LSPPosition, line, character)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Range, start, end)

// ### DidOpen ###

struct DidOpenTextDocumentParams
{
    TextDocumentItem textDocument;
};


struct DidOpenTextDocumentNotification
{
    Notification
    DidOpenTextDocumentParams params;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DidOpenTextDocumentParams, textDocument)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DidOpenTextDocumentNotification, NotificationArgs, params)

// ### DidChnage ###

struct TextDocumentContentChageEvent
{
    std::string text;
};


struct DidChangeTextDocumentParams
{
    VersionTextDocumentIdentifier textDocument;
    std::vector<TextDocumentContentChageEvent> contentChanges;
};


struct TextDocumentDidChangeNotification
{
    Notification
    DidChangeTextDocumentParams params;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextDocumentContentChageEvent, text)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DidChangeTextDocumentParams, textDocument, contentChanges)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextDocumentDidChangeNotification, NotificationArgs, params)

// ### DidSave ###

struct DidSaveTextDocumentParams
{
    TextDocumentIdentifier textDocument;
};

struct TextDocumentDidSaveNotification
{
    Notification
    DidSaveTextDocumentParams params;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DidSaveTextDocumentParams, textDocument)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextDocumentDidSaveNotification, NotificationArgs, params)

// ### Publish Diagnostics ####
#define SEVERITY_ERROR 1
#define SEVERITY_Warning 2
#define SEVERITY_INFORMATION 3
#define SEVERITY_HINT 4

struct Diagnostic
{
    Range range;
    int severity;
    std::string code;
    std::string source;
    std::string message;
};


struct PublishDiagnosticsParams
{
    std::string uri;
    std::vector<Diagnostic> diagnostics;
};

struct TextDocumentPublishDiagnostics
{
    Notification
    PublishDiagnosticsParams params;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Diagnostic, range, severity, code, source, message)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PublishDiagnosticsParams, uri, diagnostics)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextDocumentPublishDiagnostics, NotificationArgs, params)

TextDocumentPublishDiagnostics NewPublishDiagnosticsClearResponse(std::string uri)
{
    PublishDiagnosticsParams params;
    params.uri = uri;

    TextDocumentPublishDiagnostics response;
    response.jsonrpc = "2.0";
    response.method = "textDocument/publishDiagnostics";
    response.params = params;

    return response;
}

TextDocumentPublishDiagnostics NewPublishDiagnosticsResponse(std::string uri, Error* const & error, const std::vector<Warning>& warnings, std::string originSource)
{
    PublishDiagnosticsParams params;
    params.uri = uri;

    // Error
    if (error != nullptr)
    {
        Range diagnosticErrorRange;
        diagnosticErrorRange.start = {error->GetPosStart().GetLineNumber(), error->GetPosStart().GetColumn()};
        diagnosticErrorRange.end = {error->GetPosEnd().GetLineNumber(), error->GetPosEnd().GetColumn()};
        
        Diagnostic diagnosticError;
        diagnosticError.range = diagnosticErrorRange;
        diagnosticError.severity = SEVERITY_ERROR;
        diagnosticError.code = error->GetErrorName();
        diagnosticError.source = originSource;
        diagnosticError.message = error->GetErrorDetails();

        params.diagnostics.push_back(std::move(diagnosticError));
    }

    // Warnings
    for (const Warning& warning : warnings)
    {
        Range range;
        range.start = {warning.posStart.GetLineNumber(), warning.posStart.GetColumn()};
        range.end = {warning.posEnd.GetLineNumber(), warning.posEnd.GetColumn()};

        Diagnostic diagnosticWarning;
        diagnosticWarning.range = range;
        diagnosticWarning.severity = SEVERITY_Warning;
        diagnosticWarning.code = "Warning";
        diagnosticWarning.source = originSource;
        diagnosticWarning.message = warning.details;

        params.diagnostics.push_back(std::move(diagnosticWarning));
    }

    TextDocumentPublishDiagnostics response;
    response.jsonrpc = "2.0";
    response.method = "textDocument/publishDiagnostics";
    response.params = params;

    return response;
}
