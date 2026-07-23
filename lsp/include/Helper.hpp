#pragma once
#include <tuple>
#include <string_view>
#include <iostream>
#include <optional>
#include <charconv>

using byte = unsigned char;

struct Error
{
    Error(std::string msg) { this->msg = msg; }
    std::string msg;
};

struct BaseMessage
{
    std::string method;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BaseMessage, method)
};


std::tuple<std::string_view, std::string_view, bool> cut(std::string_view s, std::string_view sep) {
    size_t pos = s.find(sep);
    if (pos == std::string_view::npos) {
        return {s, "", false};
    }
    return {s.substr(0, pos), s.substr(pos + sep.length()), true};
}

std::string_view trim_prefix(std::string_view s, std::string_view prefix) {
    if (s.rfind(prefix, 0) == 0) {
        return s.substr(prefix.length());
    }
    return s;
}

std::optional<int> atoi(std::string_view s) {
    int value = 0;
    
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    
    
    if (ec == std::errc{} && ptr == s.data() + s.size()) {
        return value;
    }
    return std::nullopt;
}

std::tuple<size_t, std::string_view, bool> split(std::string_view data) {
    auto [header, content, found] = cut(data, "\r\n\r\n");
    if (!found) return {0, "", false};

    std::string_view content_length_bytes = trim_prefix(header, "Content-Length: ");
    std::optional<int> content_length = atoi(content_length_bytes);
    if (!content_length.has_value()) return {0, "", false};

    if (content.length() < static_cast<size_t>(content_length.value())) return {0, "", false};

    size_t total_length = header.length() + 4 + content_length.value();
    return {total_length, data.substr(0, total_length), true};
}