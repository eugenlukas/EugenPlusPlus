#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using byte = unsigned char;

template <typename T>
std::string EncodeMessage(const T& msg)
{
    json content = msg;

    std::string content_dump = content.dump();

    return "Content-Length: " + std::to_string(content_dump.length()) + "\r\n\r\n" + content_dump;
}

void DecodeMessage(const byte msg[])
{
    
}