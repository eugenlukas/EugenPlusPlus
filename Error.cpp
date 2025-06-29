#include "Error.hpp"
#include <string> 
#include "Helper.hpp"

Error::Error(const Position& pos_start, const Position& pos_end, const std::string& error_name, const std::string& details)
{
	this->pos_start = pos_start;
	this->pos_end = pos_end;
	this->error_name = error_name;
	this->details = details;
}

std::string Error::AsString()
{
	std::string result = error_name + ": " + details;
	result += "\nFile " + pos_start.GetFileName() + ", line " + std::to_string(pos_start.GetLineNumber() + 1);
	result += "\n\n" + Helper::StringWithArrows(pos_start.GetFileContent(), pos_start, pos_end);
	return result;
}
