#define RUN_TESTS false
#if RUN_TESTS
#include "Testing.hpp"
#endif

#include <iostream>
#include <cassert>
#include "FileLogger.h"
#include "Binder.hpp"
#include "rpc/rpc.hpp"
#include "Scanner.hpp"
#include "lsp/Initialize.hpp"
#include "lsp/TextDocument.hpp"
#include "analysis/State.hpp"
#include "analysis/Analysis.hpp"

void HandleMessage(std::string method, std::string contents, State* state)
{
    LOG("Received msg with method: " + method);

    if (method == "initialize")
    {
        InitializeRequest request;
        try 
        {
            auto j = nlohmann::json::parse(contents);
            request = j.get<InitializeRequest>();
        }
        catch (const nlohmann::json::exception& e) 
        {
            LOG(std::string("JSON parsing failed: ") + e.what());
            LOG("Got:\n" + contents);
            return;
        }
    
        LOG("Connected to: " + request.params.clientInfo.name + " " + request.params.clientInfo.version);
    
        auto msg = NewInitializeResponse(request.id);
        auto reply = EncodeMessage(msg);
    
        std::fwrite(reply.c_str(), 1, reply.size(), stdout);
        std::fflush(stdout);
    
        LOG("Send initialize reply");
    }
    else if (method == "textDocument/didOpen")
    {
        DidOpenTextDocumentNotification request;
        try 
        {
            auto j = nlohmann::json::parse(contents);
            request = j.get<DidOpenTextDocumentNotification>();
        }
        catch (const nlohmann::json::exception& e) 
        {
            LOG(std::string("JSON parsing failed: ") + e.what());
            LOG("Got:\n" + contents);
            return;
        }

        LOG("Opened: " + request.params.textDocument.uri);
        OpenDocument(state, request.params.textDocument.uri, request.params.textDocument.text);

        AnalyseForErrors(state, request.params.textDocument.uri);
    }
    else if (method == "textDocument/didChange")
    {
        TextDocumentDidChangeNotification request;
        try 
        {
            auto j = nlohmann::json::parse(contents);
            request = j.get<TextDocumentDidChangeNotification>();
        }
        catch (const nlohmann::json::exception& e) 
        {
            LOG(std::string("JSON parsing failed: ") + e.what());
            LOG("Got:\n" + contents);
            return;
        }

        LOG("Changed: " + request.params.textDocument.uri);
        for (TextDocumentContentChageEvent change : request.params.contentChanges)
        {
            UpdateDocument(state, request.params.textDocument.uri, change.text);
        }
    }
    else if (method == "textDocument/didSave")
    {
        TextDocumentDidSaveNotification request;
        try 
        {
            auto j = nlohmann::json::parse(contents);
            request = j.get<TextDocumentDidSaveNotification>();
        }
        catch (const nlohmann::json::exception& e) 
        {
            LOG(std::string("JSON parsing failed: ") + e.what());
            LOG("Got:\n" + contents);
            return;
        }

        LOG("Saved: " + request.params.textDocument.uri);
        
        AnalyseForErrors(state, request.params.textDocument.uri);
    }
}

int main()
{
#if RUN_TESTS
    TestDecode();
    TestEncode();
#endif

    LOG("LSP Started!");

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Scanner scanner;

    State* state = NewState();

    while (scanner.Scan())
    {
        std::string msg = scanner.Text();

        auto [method, contentLength, error] = DecodeMessage(msg);

        if (error != nullptr)
        {
            LOG("Got an error: " + error->msg);
            continue;
        }

        HandleMessage(method, contentLength, state);
    }

    delete state;

    return 0;
}