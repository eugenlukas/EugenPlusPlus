#pragma once
#include <memory>
#include <Nodes.hpp>
#include <unordered_map>

struct SymbolDef
{
    std::string name;
    std::string kind;
    Position posStart, posEnd;
};

struct SymbolRef
{
    std::string name;
    Position posStart, posEnd;
    SymbolDef* resolved = nullptr;
};


class Scope
{
public:
    SymbolDef* Lookup(const std::string& name)
    {
        auto it = symbols.find(name);
        if (it != symbols.end()) return &it->second;

        return parent ? parent->Lookup(name) : nullptr;
    }

private:
    Scope* parent = nullptr;
    std::unordered_map<std::string, SymbolDef> symbols;
};

class Binder
{
public:
    void Bind(std::shared_ptr<Node> node, Scope* scope);

private:
    std::vector<SymbolRef> references;
    Scope globalScope;
};