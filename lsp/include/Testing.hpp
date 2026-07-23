#pragma once
#include <nlohmann/json.hpp>
#include "rpc/rpc.hpp"
#include "FileLogger.h"

struct EncodingExample
{
    bool Testing;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(EncodingExample, Testing)
};

void TestEncode()
{
    std::string expected = "Content-Length: 16\r\n\r\n{\"Testing\":true}";
    std::string actual = EncodeMessage(EncodingExample{true});
    if (expected != actual)
        LOG("Expected: " + expected + ", Actual: " + actual);
}

void TestDecode()
{
    std::string incomingMessage = "Content-Length: 15\r\n\r\n{\"method\":\"hi\"}";
    auto [method, contentLength, error] = DecodeMessage(incomingMessage);
    if (error != nullptr)
        LOG(error->msg);

    if (contentLength != 15)
        LOG("Expected: 15, Got: " + std::to_string(contentLength));

    if (method != "hi")
        LOG("Expected: 'hi', Got: " + method);
}