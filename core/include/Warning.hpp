#pragma once
#include <string>
#include "Position.hpp"

struct Warning
{
    Warning(const Position& posStart, const Position& posEnd, const std::string& details)
    {
        this->posStart = posStart;
        this->posEnd = posEnd;
        this->details = details;
    }

    std::string details;
    Position posStart;
    Position posEnd;
};
