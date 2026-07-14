#include <iostream>
#include <cassert>
#include "Binder.hpp"
#include "rpc/rpc.hpp"
//#include <optional>
//#include <Token.hpp>
//#include <vector>

//std::optional<Token> TokenAt(const std::vector<Token>& tokens, int line, int col) {
//    for (auto& t : tokens) {
//        if (t.GetType() != TT_IDENTIFIER) continue;
//        if (PositionContains(t.GetPosStart(), t.GetPosEnd(), line, col))
//            return t;
//    }
//    return std::nullopt;
//}

struct EncodingExample
{
    bool Testing;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(EncodingExample, Testing)
};


int main()
{
    std::string expected = "Content-Length: 16\r\n\r\n{\"Testing\":true}";
    std::string actual = EncodeMessage(EncodingExample{true});
    if (expected != actual)
        std::cerr << "Expected: " + expected + ", Actual: " + actual + "\n";

    return 0;
}