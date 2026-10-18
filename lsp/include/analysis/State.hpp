#pragma once
#include <unordered_map>
#include <string>

struct State
{
    std::unordered_map<std::string, std::string> documents;
};

State* NewState()
{
    return new State;
}

void OpenDocument(State* state, std::string uri, std::string text)
{
    state->documents[uri] = text;
}

void UpdateDocument(State* state, std::string uri, std::string text)
{
    state->documents[uri] = text;
}
