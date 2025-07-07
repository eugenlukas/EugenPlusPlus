#pragma once
#include <vector>
#include <memory>
#include <iostream>
#include <variant>
#include "Nodes.hpp"

struct List;               // forward declare
class RTResult;            // forward declare

class BaseFunction
{
public:
	virtual ~BaseFunction() = default;
	virtual RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) = 0;
	virtual std::string ToString() const = 0;
};

class PrintFunction : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'PRINT'>";
	}
};

class LengthFunction : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'LENGTH'>";
	}
};

class InputStr : public BaseFunction 
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'INPUT_STR'>";
	}
};

class InputNum : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'INPUT_NUM'>";
	}
};

class Clear : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'CLEAR'>";
	}
};

class IsNum : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'IS_NUM'>";
	}
};

class IsStr : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'IS_STR'>";
	}
};

class IsList : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'IS_LIST'>";
	}
};

class IsFunc : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'IS_Func'>";
	}
};

class Append : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'APPEND'>";
	}
};

class Pop : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'POP'>";
	}
};

class Extend : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'EXTEND'>";
	}
};