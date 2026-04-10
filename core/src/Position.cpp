#include "Position.hpp"

Position::Position()
{
	this->idx = 0;
	this->ln = 0;
	this->col = 0;
}

Position::Position(int idx, int ln, int col, const std::string& fn, const std::string& ftxt)
{
	this->idx = idx;
	this->ln = ln;
	this->col = col;
	this->fn = fn;
	this->ftxt = ftxt;
}

Position Position::Advance(char current_char)
{
	idx++;
	col++;

	if (current_char == '\n')
	{
		ln++;
		col = 0;
	}

	return *this;
}

Position Position::Copy()
{
	return Position(idx, ln, col, fn, ftxt);
}
