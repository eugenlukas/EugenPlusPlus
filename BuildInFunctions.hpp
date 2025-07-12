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

class NativePrintFunction : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'PRINT'>";
	}
};

class NativeLengthFunction : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'LENGTH'>";
	}
};

class NativeInputStr : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'INPUT_STR'>";
	}
};

class NativeInputNum : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'INPUT_NUM'>";
	}
};

class NativeClear : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'CLEAR'>";
	}
};

class NativeIsNum : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'IS_NUM'>";
	}
};

class NativeIsStr : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'IS_STR'>";
	}
};

class NativeIsList : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'IS_LIST'>";
	}
};

class NativeIsFunc : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'IS_Func'>";
	}
};

class NativeAppend : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'APPEND'>";
	}
};

class NativePop : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'POP'>";
	}
};

class NativeExtend : public BaseFunction
{
	RTResult Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args) override;
	std::string ToString() const override
	{
		return "<built-in function 'EXTEND'>";
	}
};