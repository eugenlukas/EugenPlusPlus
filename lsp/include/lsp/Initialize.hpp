#pragma once
#include <memory>
#include <nlohmann/json.hpp>
#include "Message.hpp"

// ### Request ###

struct ClientInfo
{
    std::string name;
    std::string version;
};

struct InitializeRequestParams
{
    ClientInfo clientInfo;
};

struct InitializeRequest
{
    Request
    InitializeRequestParams params;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ClientInfo, name, version)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InitializeRequestParams, clientInfo)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InitializeRequest, jsonrpc, id, method, params)

// ### Response ###

struct ServerCapabilities
{
    int textDocumentSync;
};

struct ServerInfo
{
    std::string name;
    std::string version;
};

struct InitializeResult
{
    ServerCapabilities capabilities;
    ServerInfo serverInfo;
};

struct InitializeResponse
{
    Response
    InitializeResult result;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServerCapabilities, textDocumentSync)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServerInfo, name, version)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InitializeResult, capabilities, serverInfo)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InitializeResponse, jsonrpc, id, result)

InitializeResponse NewInitializeResponse(int id)
{
    InitializeResponse response;
    response.jsonrpc = "2.0";
    response.id = id;

    InitializeResult result;
    result.capabilities.textDocumentSync = 1;
    result.serverInfo.name = "eugenppLsp";
    result.serverInfo.version = "0.0.0.0.0.0-beta1.final";
    response.result = result;

    return response;
}