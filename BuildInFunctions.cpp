#include "BuildInFunctions.hpp"
#include "Helper.hpp"

RTResult NativePrintFunction::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    for (const auto& arg : args)
    {
        std::cout << Helper::Print(res.Success(arg));
    }

    return res.Success(std::nullopt);
}

RTResult NativePrintLnFunction::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    for (const auto& arg : args)
    {
        std::cout << Helper::Print(res.Success(arg));
    }

    std::cout << std::endl;
    return res.Success(std::nullopt);
}

RTResult NativeLengthFunction::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    if (args.size() != 1)
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "LENGTH() takes exactly 1 argument"));
    }

    if (std::holds_alternative<std::string>(args[0]))
    {
        return res.Success(std::make_optional(static_cast<double>(std::get<std::string>(args[0]).length())));
    }
    else if (std::holds_alternative<std::shared_ptr<List>>(args[0]))
    {
        return res.Success(std::make_optional(static_cast<double>(std::get<std::shared_ptr<List>>(args[0])->elements.size())));
    }
    else
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "LENGTH() argument must be a string or list"));
    }
}

RTResult NativeInputStr::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    std::string text = "";
    std::getline(std::cin, text);
    return RTResult().Success(text);
}

RTResult NativeInputNum::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    std::string text = "";
    double number = 0;

    while (true)
    {
        std::getline(std::cin, text);
        try
        {
            number = std::stof(text);
            break;
        }
        catch (const std::exception& ex)
        {
            std::cout << "'" << text << "' must be an number. Try again!" << std::endl;
        }
    }

    return RTResult().Success(number);
}

RTResult NativeClear::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    system("cls");

    return res.Success(std::nullopt);
}

RTResult NativeIsNum::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    for (const auto& arg : args)
    {
        if (std::holds_alternative<double>(arg))
            return res.Success(static_cast<double>(1));
        else
            return res.Success(static_cast<double>(0));
    }
}

RTResult NativeIsStr::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    for (const auto& arg : args)
    {
        if (std::holds_alternative<std::string>(arg))
            return res.Success(static_cast<double>(1));
        else
            return res.Success(static_cast<double>(0));
    }
}

RTResult NativeIsList::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    for (const auto& arg : args)
    {
        if (std::holds_alternative<std::shared_ptr<List>>(arg))
            return res.Success(static_cast<double>(1));
        else
            return res.Success(static_cast<double>(0));
    }
}

RTResult NativeIsFunc::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    for (const auto& arg : args)
    {
        if (std::holds_alternative<std::shared_ptr<BaseFunction>>(arg) || std::holds_alternative<std::shared_ptr<FuncDefNode>>(arg))
            return res.Success(static_cast<double>(1));
        else
            return res.Success(static_cast<double>(0));
    }
}

RTResult NativeAppend::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    if (args.size() != 2)
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "APPEND() takes exactly 2 arguments"));
    }

    if (!std::holds_alternative<std::shared_ptr<List>>(args[0]))
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "First argument must be a list"));
    }

    auto list = std::get<std::shared_ptr<List>>(args[0]);
    list->elements.push_back(args[1]);
    return res.Success(std::nullopt);
}

RTResult NativePop::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    if (args.size() < 1 || args.size() > 2)
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "POP() takes 1 or 2 arguments"));
    }

    if (!std::holds_alternative<std::shared_ptr<List>>(args[0]))
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "First argument must be a list"));
    }

    auto list = std::get<std::shared_ptr<List>>(args[0]);
    double index = -1;

    if (args.size() == 2)
    {
        if (!std::holds_alternative<double>(args[1]))
        {
            return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "Second argument must be a number"));
        }
        index = std::get<double>(args[1]);
    }

    if (list->elements.empty())
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "Cannot pop from empty list"));
    }

    std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>> popped_value;

    if (index == -1)
    {
        popped_value = list->elements.back();
        list->elements.pop_back();
    }
    else
    {
        int idx = static_cast<int>(index);
        if (idx < 0 || idx >= static_cast<int>(list->elements.size()))
        {
            return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "Index out of bounds"));
        }
        popped_value = list->elements[idx];
        list->elements.erase(list->elements.begin() + idx);
    }

    return res.Success(popped_value);
}

RTResult NativeExtend::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    if (args.size() != 2)
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "EXTEND() takes exactly 2 arguments"));
    }

    if (!std::holds_alternative<std::shared_ptr<List>>(args[0]))
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "First argument must be a list"));
    }

    if (!std::holds_alternative<std::shared_ptr<List>>(args[1]))
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "Second argument must be a list"));
    }

    auto listA = std::get<std::shared_ptr<List>>(args[0]);
    auto listB = std::get<std::shared_ptr<List>>(args[1]);

    listA->elements.insert(listA->elements.end(), listB->elements.begin(), listB->elements.end());

    return res.Success(std::nullopt);
}

RTResult NativeSystem::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    if (args.size() != 1)
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "SYSTEM() takes exactly 1 argument"));
    }

    if (!std::holds_alternative<std::string>(args[0]))
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "Argument must be a string"));
    }

    system(std::get<std::string>(args[0]).c_str());

    return res.Success(std::nullopt);
}

RTResult NativeRandom::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    if (args.size() != 2)
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "RANDOM() takes exactly 2 arguments"));
    }

    if (!std::holds_alternative<double>(args[0]))
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "First argument must be a number"));
    }

    if (!std::holds_alternative<double>(args[1]))
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "Second argument must be a number"));
    }

    int min = std::get<double>(args[0]);
    int max = std::get<double>(args[1]);
    int randVal = min + (std::rand() % (max - min + 1));

    return res.Success(static_cast<double>(randVal));
}

RTResult NativeRandomize::Execute(std::vector<std::variant<double, std::string, std::shared_ptr<FuncDefNode>, std::shared_ptr<List>, std::shared_ptr<BaseFunction>>> args)
{
    RTResult res;

    if (args.size() > 1)
    {
        return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "RANDOMIZE() takes no arguments or a number as seed"));
    }

    if (args.size() == 1)
    {
        if (std::holds_alternative<double>(args[0]))
            std::srand(static_cast<unsigned int>(std::get<double>(args[0])));
        else
            return res.Failure(std::make_unique<RuntimeError>(Position(), Position(), "Seed must be a number"));
    }
    else
        std::srand(static_cast<unsigned int>(std::time(nullptr)));

    return res.Success(std::nullopt);
}
