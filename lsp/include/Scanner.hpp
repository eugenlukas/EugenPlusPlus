#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <string_view>

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
        
            // Read one character instead of waiting for 4096 bytes.
            int ch = std::cin.get();
        
            if (ch == std::char_traits<char>::eof())
            {
                return false;
            }
        
            buffer.push_back(static_cast<char>(ch));
        }
    }

    std::string Text() const { return currentToken; }

private:
    std::vector<char> buffer;
    std::string currentToken;
    static constexpr size_t CHUNK_SIZE = 4096;
};