#pragma once
#include <iostream>
#include <vector>
#include "Token.hpp"

class Helper
{
public:
	static std::string TokenVectorToString(std::vector<Token> vector)
	{
		std::string result = "[";

		for (size_t i = 0; i < vector.size(); i++)
		{
			result += vector[i].Repr();
			if (i != vector.size() - 1)
				result += ", ";
		}
		result += "]";

		return result;
	}

    static std::string StringWithArrows(const std::string& text, Position pos_start, Position pos_end)
    {
        std::string result;

        // Find start of the line
        size_t idxStart = text.rfind('\n', pos_start.GetIdx());
        idxStart = (idxStart == std::string::npos) ? 0 : idxStart + 1;

        // Find end of the line
        size_t idxEnd = text.find('\n', idxStart);
        idxEnd = (idxEnd == std::string::npos) ? text.length() : idxEnd;

        // Extract the line
        std::string line = text.substr(idxStart, idxEnd - idxStart);

        // Append the line itself
        result += line + "\n";

        // Caret line
        int colStart = pos_start.GetColumn();
        int colEnd = pos_end.GetColumn();

        // Defensive: ensure caret at least one character
        if (colEnd < colStart)
            colEnd = colStart;

        result += std::string(colStart, ' ');
        result += std::string(std::max(colEnd - colStart, 1), '^');

        return result;
    }
};