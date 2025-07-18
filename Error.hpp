#pragma once
#include <iostream>
#include "Position.hpp"

class Error 
{
public:
	Error();
	Error(const Position& pos_start, const Position& pos_end, const std::string& error_name, const std::string& details);

	std::string AsString();

private:
	Position pos_start;
	Position pos_end;
	std::string error_name;
	std::string details;

};

class IllegalCharError : public Error
{
public:
	IllegalCharError(const Position& pos_start, const Position& pos_end, std::string details) : Error(pos_start, pos_end, "Illegal Character", details) {}
};

class ExpectedCharError : public Error
{
public:
	ExpectedCharError(const Position& pos_start, const Position& pos_end, std::string details) : Error(pos_start, pos_end, "Expected Character", details) {}
};

class InvalidSyntaxError : public Error
{
public:
	InvalidSyntaxError(const Position& pos_start, const Position& pos_end, std::string details="") : Error(pos_start, pos_end, "Invalid Syntax", details) {}
};

class RuntimeError : public Error
{
public:
	RuntimeError(const Position& pos_start, const Position& pos_end, std::string details="") : Error(pos_start, pos_end, "Runtime Error", details) {}
};