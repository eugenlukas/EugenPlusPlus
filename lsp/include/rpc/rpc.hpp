#pragma once
#include <nlohmann/json.hpp>
#include "Helper.hpp"

using json = nlohmann::json;

template <typename T>
std::string EncodeMessage(const T& msg)
{
    json content = msg;

    std::string content_dump = content.dump();

    return "Content-Length: " + std::to_string(content_dump.length()) + "\r\n\r\n" + content_dump;
}

std::tuple<std::string, int, std::shared_ptr<Error>> DecodeMessage(std::string_view msg)
{
    auto [header, content, found] = cut(msg, "\r\n\r\n");

    if (!found)
        return {"", 0, std::make_shared<Error>("Did not found seperator\n")};

    std::string_view contentLengthBytes = trim_prefix(header, "Content-Length: ");
    auto contentLength = atoi(contentLengthBytes);
    if (!contentLength.has_value())
        return {"", 0, std::make_shared<Error>("Content Length could not be converted to an integer")};

    BaseMessage baseMessage;
    std::string_view jsonPayload = content.substr(0, contentLength.value());
    try 
    {
        auto j = nlohmann::json::parse(jsonPayload);
        baseMessage = j.get<BaseMessage>();
    }
    catch (const nlohmann::json::exception& e) 
    {
        return {"", 0, std::make_shared<Error>(std::string("JSON parsing failed: ") + e.what())};
    }

    return {baseMessage.method, contentLength.value(), nullptr};
}