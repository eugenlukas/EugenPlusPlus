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
};