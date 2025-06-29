#pragma once
#include <iostream>

class Position
{
public:
	Position();
	Position(int idx, int ln, int col, const std::string& fn, const std::string& ftxt);

	Position Advance(char current_char);
	Position Copy();

	int GetIdx() { return idx; }
	std::string GetFileName() { return fn; }
	int GetLineNumber() { return ln; }

private:
	int idx;
	int ln;
	int col;
	std::string fn;
	std::string ftxt;

};