#pragma once
#include <iostream>
#include <vector>
#include "Token.hpp"
#include "Interpreter.hpp"
#include <sstream>
#include <iomanip>

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

    static std::string Print(RTResult result)
    {
        if (result.GetValue().has_value())
        {
            auto val = result.GetValue().value();
            if (std::holds_alternative<double>(val))                        // Print number
            {
                double number = std::get<double>(val);

                if (number == static_cast<int>(number))                         // Print number as int
                    return std::to_string(static_cast<int>(number));
                else                                                            // Print number as double
                {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(15) << std::get<double>(val);
                    return oss.str();
                }
            }
            else if (std::holds_alternative<std::string>(val))              // Print string
                return (std::get<std::string>(val));
            else if (std::holds_alternative<std::shared_ptr<List>>(val))    // Print list
            {
                const auto& list = std::get<std::shared_ptr<List>>(val);

                if (list->elements.size() == 1)                                 // Print the one result in the list directly
                    return Print(RTResult().Success(list->elements[0]));
                else if (list->elements.size() == 0)                            // Print empty string so it does not print []
                    return "";
                else                                                            // Print the results in the list as list
                {
                    std::string result = "[";
                    for (size_t i = 0; i < list->elements.size(); ++i)
                    {
                        result += Print(RTResult().Success(list->elements[i]));
                        if (i != list->elements.size() - 1)
                        {
                            result += ", ";
                        }
                    }
                    result += "]";
                    return result;
                }
            }
            else if (std::holds_alternative<std::shared_ptr<BaseFunction>>(val))              // Print build in function name
                return (std::get<std::shared_ptr<BaseFunction>>(val)->ToString());
            else if (std::holds_alternative<std::shared_ptr<FuncDefNode>>(val))              // Print function name
                return (std::get<std::shared_ptr<FuncDefNode>>(val)->Repr());
        }
        else
            return "";
    }
};