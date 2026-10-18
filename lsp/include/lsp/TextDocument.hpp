#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Message.hpp"
#include "../../../core/include/Error.hpp"

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

TextDocumentPublishDiagnostics NewPublishDiagnosticsErrorResponse(std::string uri, Error* const & error, std::string errorSource)
{
    Range diagnosticRange;
    diagnosticRange.start = {error->GetPosStart().GetLineNumber(), error->GetPosStart().GetColumn()};
    diagnosticRange.end = {error->GetPosEnd().GetLineNumber(), error->GetPosEnd().GetColumn()};

    Diagnostic diagnostic;
    diagnostic.range = diagnosticRange;
    diagnostic.severity = SEVERITY_ERROR;
    diagnostic.code = error->GetErrorName();
    diagnostic.source = errorSource;
    diagnostic.message = error->GetErrorDetails();

    PublishDiagnosticsParams params;
    params.uri = uri;
    params.diagnostics.push_back(diagnostic);

    TextDocumentPublishDiagnostics response;
    response.jsonrpc = "2.0";
    response.method = "textDocument/publishDiagnostics";
    response.params = params;

    return response;
}
