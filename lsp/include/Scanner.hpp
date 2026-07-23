#pragma once
#include <vector>
#include <string>
#include "Helper.hpp"

class Scanner
{
public:
    bool Scan()
    {
        while (true)
        {
            std::string_view view(buffer.data(), buffer.size());
            auto [advance, token, found] = split(view);

            if (found)
            {
                currentToken = std::string(token);
                buffer.erase(buffer.begin(), buffer.begin() + advance);
                return true;
            }

            char readBuffer[CHUNK_SIZE];
            std::cin.read(readBuffer, CHUNK_SIZE);
            std::streamsize bytesRead = std::cin.gcount();

            if (bytesRead <= 0)
                return false;

            buffer.insert(buffer.end(), readBuffer, readBuffer + bytesRead);
        }
    }

    std::string Text() const { return currentToken; }

private:
    std::vector<char> buffer;
    std::string currentToken;
    static constexpr size_t CHUNK_SIZE = 4096;
};