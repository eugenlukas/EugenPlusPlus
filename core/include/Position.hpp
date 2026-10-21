#pragma once
#include <iostream>

class Position
{
public:
	Position();
	Position(int idx, int ln, int col, const std::string& fn, const std::string& ftxt);

	Position Advance(char current_char=NULL);
	Position Copy();

	int GetIdx() const { return idx; }
	std::string GetFileName() const { return fn; }
	int GetLineNumber() const { return ln; }
	std::string GetFileContent() const { return ftxt; }
	int GetColumn() const { return col; }

private:
	int idx;
	int ln;
	int col;
	std::string fn;
	std::string ftxt;

};