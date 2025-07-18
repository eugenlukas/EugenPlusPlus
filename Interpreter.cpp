#include "Interpreter.hpp"
#include <functional>
#include <fstream>
#include <sstream>
#include "Lexer.hpp"
#include "Parser.hpp"
#include <filesystem>

RTResult Interpreter::Visit(std::shared_ptr<Node> node)
{
    if (auto number = dynamic_cast<NumberNode*>(node.get()))
        return Visit_NumberNode(*number);
    if (auto string = dynamic_cast<StringNode*>(node.get()))
        return Visit_StringNode(*string);
    if (auto binOp = dynamic_cast<BinOpNode*>(node.get()))
        return Visit_BinOpNode(*binOp);
    if (auto unary = dynamic_cast<UnaryOpNode*>(node.get()))
        return Visit_UnaryOpNode(*unary);
    if (auto varAccess = dynamic_cast<VarAccessNode*>(node.get()))
        return Visit_VarAccessNode(*varAccess);
    if (auto varAssign = dynamic_cast<VarAssignNode*>(node.get()))
        return Visit_VarAssignNode(*varAssign);
    if (auto varIf = dynamic_cast<IfNode*>(node.get()))
        return Visit_IfNode(*varIf);
    if (auto for_ = dynamic_cast<ForNode*>(node.get()))
        return Visit_ForNode(*for_);
    if (auto while_ = dynamic_cast<WhileNode*>(node.get()))
        return Visit_WhileNode(*while_);
    if (auto funcDef = dynamic_cast<FuncDefNode*>(node.get()))
        return Visit_FuncDefNode(*funcDef);
    if (auto call = dynamic_cast<CallNode*>(node.get()))
        return Visit_CallNode(*call);
    if (auto list = dynamic_cast<ListNode*>(node.get()))
        return Visit_ListNode(*list);
    if (auto return_ = dynamic_cast<ReturnNode*>(node.get()))
        return Visit_ReturnNode(*return_);
    if (auto continue_ = dynamic_cast<ContinueNode*>(node.get()))
        return Visit_ContinueNode(*continue_);
    if (auto break_ = dynamic_cast<BreakNode*>(node.get()))
        return Visit_BreakNode(*break_);
    if (auto import_ = dynamic_cast<ImportNode*>(node.get()))
        return Visit_ImportNode(*import_);

    return RTResult().Failure(std::make_unique<RuntimeError>(Position(), Position(), "Unknown node type"));
}

RTResult Interpreter::Visit_NumberNode(NumberNode& node)
{
    auto val = node.GetToken().GetValue();
    double num = 0;

    if (std::holds_alternative<int>(val))
        num = static_cast<double>(std::get<int>(val));
    else if (std::holds_alternative<double>(val))
        num = std::get<double>(val);

    return RTResult().Success(num);
}

RTResult Interpreter::Visit_StringNode(StringNode& node)
{
    return RTResult().Success(std::get<std::string>(node.GetToken().GetValue()));
}

RTResult Interpreter::Visit_ListNode(ListNode& node)
{
    RTResult res;

    std::vector<ListValue> elements;
    for (auto elementNode : node.GetElementNodes())
    {
        auto result = Visit(elementNode);
        if (result.ShouldReturn())
            return result;

        if (result.GetValue().has_value())
            elements.push_back(result.GetValue().value());
    }
    return res.Success(std::make_shared<List>(List(elements)));
}

RTResult Interpreter::Visit_BinOpNode(BinOpNode& node)
{
    auto left = Visit(node.GetLeftNode());
    auto right = Visit(node.GetRightNode());

    if (left.ShouldReturn())
        return left;
    if (right.ShouldReturn())
        return right;

    auto l = left.GetValue().value();
    auto r = right.GetValue().value();

    Position pos_start = node.GetOpToken().GetPosStart();
    Position pos_end = node.GetOpToken().GetPosEnd();

    if (node.GetOpToken().GetType() == TT_PLUS)  // Handle addition
    {
        // String + String
        if (std::holds_alternative<std::string>(l) && std::holds_alternative<std::string>(r))
        {
            std::string result = std::get<std::string>(l) + std::get<std::string>(r);
            return RTResult().Success(result);
        }
        // Number + Number
        else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))
        {
            double result = std::get<double>(l) + std::get<double>(r);
            return RTResult().Success(result);
        }
        //List + ListVar
        else if (std::holds_alternative<std::shared_ptr<List>>(l))
        {
            List result(std::get<std::shared_ptr<List>>(l)->elements);
            result.elements.push_back(ListValue(r));
            return RTResult().Success(std::make_shared<List>(result));
        }

    }
    else if (node.GetOpToken().GetType() == TT_MUL)  // Handle multiplication
    {
        // String * Number
        if (std::holds_alternative<std::string>(l) && std::holds_alternative<double>(r))
        {
            std::string str = std::get<std::string>(l);
            int times = static_cast<int>(std::get<double>(r));
            std::string result;
            for (int i = 0; i < times; ++i)
                result += str;
            return RTResult().Success(result);
        }
        // Number * String
        else if (std::holds_alternative<double>(l) && std::holds_alternative<std::string>(r))
        {
            int times = static_cast<int>(std::get<double>(l));
            std::string str = std::get<std::string>(r);
            std::string result;
            for (int i = 0; i < times; ++i)
                result += str;
            return RTResult().Success(result);
        }
        // Number * Number
        else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))
        {
            double result = std::get<double>(l) * std::get<double>(r);
            return RTResult().Success(result);
        }
        //List * List
        else if (std::holds_alternative<std::shared_ptr<List>>(l) && std::holds_alternative<std::shared_ptr<List>>(r))
        {
            List result(std::get<std::shared_ptr<List>>(l)->elements);

            for (ListValue listVar : std::get<std::shared_ptr<List>>(r)->elements)
                result.elements.push_back(listVar);

            return RTResult().Success(std::make_shared<List>(result));
        }
    }
    else if (node.GetOpToken().GetType() == TT_AT)  // Handle list indexing with '@'
    {
        if (std::holds_alternative<std::shared_ptr<List>>(l) && std::holds_alternative<double>(r))
        {
            auto listVal = std::get<std::shared_ptr<List>>(l);
            int index = static_cast<int>(std::get<double>(r));

            if (index < 0 || index >= listVal->elements.size())
            {
                return RTResult().Failure(
                    std::make_unique<RuntimeError>(
                        pos_start, pos_end, "Index out of bounds in list"
                    )
                );
            }

            return RTResult().Success(ListValue(listVal->elements[index]));
        }
        else
        {
            return RTResult().Failure(
                std::make_unique<RuntimeError>(
                    pos_start, pos_end, "Invalid operands for list indexing"
                )
            );
        }
    }
    else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))  // Other numeric operators (require numbers only)
    {
        double lNum = std::get<double>(l);
        double rNum = std::get<double>(r);

        if (node.GetOpToken().GetType() == TT_MINUS)
            return RTResult().Success(lNum - rNum);
        else if (node.GetOpToken().GetType() == TT_DIV)
        {
            if (rNum == 0)
                return RTResult().Failure(std::make_unique<RuntimeError>(pos_start, pos_end, "Division by zero"));
            return RTResult().Success(lNum / rNum);
        }
        else if (node.GetOpToken().GetType() == TT_POW)
            return RTResult().Success(pow(lNum, rNum));
        else if (node.GetOpToken().GetType() == TT_EQEQ)
            return RTResult().Success(SymbolValue(static_cast<double>(lNum == rNum)));
        else if (node.GetOpToken().GetType() == TT_NEQ)                                      
            return RTResult().Success(SymbolValue(static_cast<double>(lNum != rNum)));
        else if (node.GetOpToken().GetType() == TT_LT)                                      
            return RTResult().Success(SymbolValue(static_cast<double>(lNum < rNum)));
        else if (node.GetOpToken().GetType() == TT_GT)                                      
            return RTResult().Success(SymbolValue(static_cast<double>(lNum > rNum)));
        else if (node.GetOpToken().GetType() == TT_LTEQ)                                    
            return RTResult().Success(SymbolValue(static_cast<double>(lNum <= rNum)));
        else if (node.GetOpToken().GetType() == TT_GTEQ)                                   
            return RTResult().Success(SymbolValue(static_cast<double>(lNum >= rNum)));
        else if (node.GetOpToken().Matches(TT_KEYWORD, "AND"))                             
            return RTResult().Success(SymbolValue(static_cast<double>(lNum && rNum)));
        else if (node.GetOpToken().Matches(TT_KEYWORD, "OR"))                              
            return RTResult().Success(SymbolValue(static_cast<double>(lNum || rNum)));
    }
    else if (std::holds_alternative<std::shared_ptr<List>>(l) && std::holds_alternative<double>(r))
    {
        int index = static_cast<int>(std::get<double>(r));
        List result(std::get<std::shared_ptr<List>>(l)->elements);

        if (index < 0 || index >= result.elements.size())
            return RTResult().Failure(std::make_unique<RuntimeError>(pos_start, pos_end, "Index out of bounce in list deletion"));

        result.elements.erase(result.elements.begin() + std::get<double>(r));
        return RTResult().Success(std::make_shared<List>(result));
    }

    return RTResult().Failure(std::make_unique<RuntimeError>(pos_start, pos_end, "Unsupported operand types for binary operation"));
}

RTResult Interpreter::Visit_UnaryOpNode(UnaryOpNode& node)
{
    RTResult res_num = Visit(node.GetNode());
    if (res_num.ShouldReturn()) return res_num;

    double num = std::get<double>(res_num.GetValue().value());
    std::string op_type = node.GetOpToken().GetType();

    if (op_type == TT_MINUS)
        return RTResult().Success(-num);
    if (op_type == TT_PLUS)
        return RTResult().Success(+num);
    if (node.GetOpToken().Matches(TT_KEYWORD, "NOT"))
        return RTResult().Success(SymbolValue(static_cast<double>(num == 0 ? 1 : 0)));

    return RTResult().Failure(
        std::make_unique<RuntimeError>(
            node.GetOpToken().GetPosStart(),
            node.GetOpToken().GetPosEnd(),
            "Unknown unary operator"
        )
    );
}

RTResult Interpreter::Visit_VarAccessNode(VarAccessNode& node)
{
    RTResult res;

    std::string varName = std::get<std::string>(node.GetVarNameToken().GetValue());

    // If namespaced (like Test::func1)
    if (node.IsNamespaced())
    {
        std::string moduleAlias = node.GetModuleAlias().value();

        auto it = importedModules.find(moduleAlias);
        if (it == importedModules.end())
        {
            return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Module '" + moduleAlias + "' not found"));
        }

        auto moduleSymbols = it->second;
        auto value = moduleSymbols->Get(varName);
        if (!value.has_value())
        {
            return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "'" + varName + "' not found in module '" + moduleAlias + "'"));
        }

        return res.Success(value);
    }

    // Normal access
    auto value = symbolTable.Get(varName);

    if (!value.has_value())
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "'" + varName + "' is not defined"));

    if (std::holds_alternative<double>(value.value()))
        return res.Success(std::get<double>(value.value()));
    else if (std::holds_alternative<std::string>(value.value()))
        return res.Success(std::get<std::string>(value.value()));
    else if (std::holds_alternative<std::shared_ptr<List>>(value.value()))
        return res.Success(std::get<std::shared_ptr<List>>(value.value()));
    else if (std::holds_alternative<std::shared_ptr<BaseFunction>>(value.value()))
        return res.Success(std::get<std::shared_ptr<BaseFunction>>(value.value()));
    else if (std::holds_alternative<std::shared_ptr<FuncDefNode>>(value.value()))
        return res.Success(std::get<std::shared_ptr<FuncDefNode>>(value.value()));
    else
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Variable is not a number, a string, a List or a Function"));
}

RTResult Interpreter::Visit_VarAssignNode(VarAssignNode& node)
{
    std::string varName = std::get<std::string>(node.GetVarNameToken().GetValue());
    RTResult res_value = Visit(node.GetValueNode());

    if (res_value.ShouldReturn())
        return res_value;

    if (std::holds_alternative<double>(res_value.GetValue().value()))
        symbolTable.Set(varName, std::get<double>(res_value.GetValue().value()));
    else if (std::holds_alternative<std::string>(res_value.GetValue().value()))
        symbolTable.Set(varName, std::get<std::string>(res_value.GetValue().value()));
    else if (std::holds_alternative<std::shared_ptr<List>>(res_value.GetValue().value()))
        symbolTable.Set(varName, std::get<std::shared_ptr<List>>(res_value.GetValue().value()));

    return res_value.Success(res_value.GetValue().value());
}

RTResult Interpreter::Visit_IfNode(IfNode& node)
{
    RTResult res;

    for (IfCase& ifCase : node.GetCases())
    {
        RTResult conditionValue = Visit(ifCase.GetCondition());
        if (conditionValue.ShouldReturn())
            return conditionValue;

        if (std::get<double>(conditionValue.GetValue().value()) != 0)
        {
            RTResult exprValue = Visit(ifCase.GetExpr());
            if (exprValue.ShouldReturn())
                return exprValue;

            if (exprValue.GetValue().has_value() == false)
                return res.Success(std::nullopt);

            if (!ifCase.GetShouldReturnNull())
                return res.Success(exprValue.GetValue().value());
            else
                return res.Success(std::nullopt);
        }
    }

    if (node.GetElseCase() != nullptr)
    {
        RTResult elseValue = Visit(node.GetElseCase());
        if (elseValue.ShouldReturn())
            return elseValue;

        if (elseValue.GetValue().has_value() == false)
            return res.Success(std::nullopt);

        /*if (!node.GetElseCase())
            return res.Success(elseValue.GetValue().value());
        else*/
            return res.Success(std::nullopt);
    }

    return res.Success(std::nullopt);
}

RTResult Interpreter::Visit_ForNode(ForNode& node)
{
    RTResult res;
    std::vector<ListValue> elements;

    RTResult startValue = Visit(node.GetStartValueNode());
    if (startValue.ShouldReturn())
        return startValue;

    RTResult endValue = Visit(node.GetEndValueNode());
    if (endValue.ShouldReturn())
        return endValue;

    RTResult stepValue;
    if (node.GetStepValueNode() != nullptr)
    {
        stepValue = Visit(node.GetStepValueNode());
        if (stepValue.ShouldReturn())
            return startValue;
    }
    else
        stepValue.SetValue(static_cast<double>(1));

    std::function<bool(int)> condition;

    if (std::get<double>(stepValue.GetValue().value()) >= 0)
        condition = [=](int i) { return i < std::get<double>(endValue.GetValue().value()); };
    else
        condition = [=](int i) { return i > std::get<double>(endValue.GetValue().value()); };

    for (int i = std::get<double>(startValue.GetValue().value()); condition(i); i += std::get<double>(stepValue.GetValue().value()))
    {
        symbolTable.Set(std::get<std::string>(node.GetVarNameTok().GetValue()), static_cast<double>(i));

        res = Visit(node.GetBodyNode());
        if (res.ShouldReturn() && !res.GetLoopShouldContinue() && !res.GetLoopShouldBreak())
            return res;

        if (res.GetLoopShouldContinue())
            continue;

        if (res.GetLoopShouldBreak())
            break;

        elements.push_back(res.GetValue().value());
    }

    if (!node.GetShouldReturnNull())
        return res.Success(std::make_shared<List>(elements));
    else
        return res.Success(std::nullopt);
}

RTResult Interpreter::Visit_WhileNode(WhileNode& node)
{
    RTResult res;
    std::vector<ListValue> elements;

    while (true)
    {
        auto condition = Visit(node.GetConditionNode());
        if (condition.ShouldReturn())
            return condition;

        if (std::get<double>(condition.GetValue().value()) == 0)
            break;

        res = Visit(node.GetBodyNode());
        if (res.ShouldReturn() && !res.GetLoopShouldContinue() && !res.GetLoopShouldBreak())
            return res;

        if (res.GetLoopShouldContinue())
            continue;

        if (res.GetLoopShouldBreak())
            break;

        elements.push_back(res.GetValue().value());
    }

    if (!node.GetShouldReturnNull())
        return res.Success(std::make_shared<List>(elements));
    else
        return res.Success(std::nullopt);
}

RTResult Interpreter::Visit_FuncDefNode(FuncDefNode& node)
{
    RTResult res;

    if (node.GetVarNameTok().has_value())
    {
        std::string funcName = std::get<std::string>(node.GetVarNameTok().value().GetValue());
        symbolTable.Set(funcName, std::make_shared<FuncDefNode>(node));
    }

    return res.Success(std::nullopt);
}

RTResult Interpreter::Visit_CallNode(CallNode& node)
{
    RTResult res;
    std::string funcName;
    std::optional<std::string> moduleAlias;
    std::optional<SymbolValue> funcValue;

    if (auto varAccess = dynamic_cast<VarAccessNode*>(node.GetNodeToCall().get()))
    {
        funcName = std::get<std::string>(varAccess->GetVarNameToken().GetValue());
        moduleAlias = varAccess->GetModuleAlias();

        if (moduleAlias.has_value())
        {
            auto it = importedModules.find(*moduleAlias);
            if (it == importedModules.end())
                return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Module '" + *moduleAlias + "' not found"));

            funcValue = it->second->Get(funcName);
        }
        else
            funcValue = symbolTable.Get(funcName);
    }
    else
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Invalid function name"));

    if (!funcValue.has_value())
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Function '" + funcName + "' not found"));

    // Handle user-defined functions
    if (std::holds_alternative<std::shared_ptr<FuncDefNode>>(funcValue.value()))
    {
        auto funcNodePtr = std::get<std::shared_ptr<FuncDefNode>>(funcValue.value());

        if (node.GetArgNodes().size() != funcNodePtr->GetArgNameToks().size())
            return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Incorrect number of arguments"));

        // Save current symbol table
        SymbolTable localSymbolTable(&symbolTable);

        // Assign arguments
        for (size_t i = 0; i < node.GetArgNodes().size(); ++i)
        {
            auto argRes = Visit(node.GetArgNodes()[i]);
            if (argRes.ShouldReturn()) return argRes;

            std::string argName = std::get<std::string>(funcNodePtr->GetArgNameToks()[i].GetValue());
            auto argResVal = argRes.GetValue().value();

            if (std::holds_alternative<double>(argResVal))
                localSymbolTable.Set(argName, std::get<double>(argResVal));
            else if (std::holds_alternative<std::string>(argResVal))
                localSymbolTable.Set(argName, std::get<std::string>(argResVal));
            else if (std::holds_alternative<std::shared_ptr<List>>(argResVal))
                localSymbolTable.Set(argName, std::get<std::shared_ptr<List>>(argResVal));
        }

        // Execute function body
        Interpreter funcInterpreter(localSymbolTable);
        funcInterpreter.SetMainFilePath(mainFilePath);
        auto result = funcInterpreter.Visit(funcNodePtr->GetBodyNode());
        if (result.HasError()) return result;
        if (result.GetLoopShouldBreak() || result.GetLoopShouldContinue())
            return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Cannot use 'break' or 'continue' outside of a loop"));

        if (result.GetFuncReturnValue().has_value())
            return res.Success(result.GetFuncReturnValue());
        else if (funcNodePtr->GetShouldAutoReturn())
            return res.Success(result.GetValue());
        else
            return res.Success(std::nullopt);
    }
    // Handle built-in functions
    else if (std::holds_alternative<std::shared_ptr<BaseFunction>>(funcValue.value()))
    {
        auto func = std::get<std::shared_ptr<BaseFunction>>(funcValue.value());

        std::vector<SymbolValue> args;
        for (auto& argNode : node.GetArgNodes())
        {
            auto argRes = Visit(argNode);
            if (argRes.ShouldReturn()) return argRes;

            auto val = argRes.GetValue().value();
            if (std::holds_alternative<double>(val) || std::holds_alternative<std::string>(val) || std::holds_alternative<std::shared_ptr<List>>(val) || std::holds_alternative<std::shared_ptr<BaseFunction>>(val) || std::holds_alternative<std::shared_ptr<FuncDefNode>>(val))
            {
                args.push_back(val);
            }
            else
            {
                return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Unsupported argument type for built-in function"));
            }
        }

        auto result = func->Execute(args);
        if (result.ShouldReturn()) return result;

        return res.Success(result.GetValue());
    }
    else
    {
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Function '" + funcName + "' not callable"));
    }
}

RTResult Interpreter::Visit_ReturnNode(ReturnNode& node)
{
    RTResult res;

    if (node.GetNodeToReturn().has_value())
    {
        res = Visit(node.GetNodeToReturn().value());
        if (res.ShouldReturn())
            return res;

        return res.SuccessReturn(res.GetValue().value());
    }
    else
        return res.SuccessReturn(std::nullopt);
}

RTResult Interpreter::Visit_ContinueNode(ContinueNode& node)
{
    return RTResult().SuccessContinue();
}

RTResult Interpreter::Visit_BreakNode(BreakNode& node)
{
    return RTResult().SuccessBreak();
}

RTResult Interpreter::Visit_ImportNode(ImportNode& node)
{
    RTResult res;

    std::filesystem::path filePath(std::get<std::string>(node.GetFilepathToken().GetValue()));
    std::filesystem::path MainFilePath = mainFilePath;

    // If path is relative, resolve it based on importing file's directory
    if (!filePath.is_absolute())
        filePath = MainFilePath.parent_path().string() + "\\" + std::get<std::string>(node.GetFilepathToken().GetValue());

    if (!std::filesystem::exists(filePath))
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Import file not found: " + filePath.string()));

    // Normalize (e.g. resolve "..", ".")
    filePath = std::filesystem::canonical(filePath);

    std::ifstream file(filePath);
    if (!file.is_open())
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), "Could not open import file: " + filePath.string()));

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string fileContent = buffer.str();
    file.close();

    Lexer lexer(filePath.string(), fileContent);
    auto lexResult = lexer.MakeTokens();
    if (lexResult.error != nullptr)
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), lexResult.error->AsString()));

    Parser parser(lexResult.tokens);
    auto parseResult = parser.Parse();
    if (parseResult.HasError())
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), parseResult.GetError()));

    std::shared_ptr<Node> tree = parseResult.GetNode();

    auto importSymbolTable = std::make_shared<SymbolTable>(&symbolTable);

    Interpreter importInterpreter(*importSymbolTable);
    importInterpreter.SetMainFilePath(filePath.string());
    RTResult importResult = importInterpreter.Visit(tree);
    if (importResult.HasError())
        return res.Failure(std::make_unique<RuntimeError>(node.GetPosStart(), node.GetPosEnd(), importResult.GetError()));

    importedModules[node.GetAlias()] = importSymbolTable;

    return res.Success(std::nullopt);
}

RTResult& RTResult::Success(std::optional<SymbolValue> value)
{
    Reset();
    this->value = value;
    return *this;
}

RTResult& RTResult::SuccessReturn(std::optional<SymbolValue> value)
{
    Reset();
    this->funcReturnValue = value;
    return *this;
}

RTResult& RTResult::SuccessContinue()
{
    Reset();
    loopShouldContinue = true;
    return *this;
}

RTResult& RTResult::SuccessBreak()
{
    Reset();
    loopShouldBreak = true;
    return *this;
}

RTResult& RTResult::Failure(std::unique_ptr<Error> error)
{
    Reset();
    this->error = std::move(error);
    return *this;
}

bool RTResult::ShouldReturn()
{
    if (error != nullptr || funcReturnValue.has_value() || loopShouldContinue || loopShouldBreak)
        return true;
    else
        return false;
}

void RTResult::Reset()
{
    error = nullptr;
    value = std::nullopt;
    funcReturnValue = std::nullopt;
    loopShouldContinue = false;
    loopShouldBreak = false;
}
