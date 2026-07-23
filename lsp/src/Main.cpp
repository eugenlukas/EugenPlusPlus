#define RUN_TESTS true

#include <iostream>
#include <cassert>
#include "Binder.hpp"
#include "rpc/rpc.hpp"
#include "Scanner.hpp"
#if RUN_TESTS
#include "Testing.hpp"
#endif

void HandleMessage(std::string msg)
{
    LOG(msg);
}

int main()
{
#if RUN_TESTS
    TestDecode();
    TestEncode();
#endif

    LOG("LSP Started!");

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Scanner scanner;

    while (scanner.Scan())
    {
        std::string msg = scanner.Text();
        HandleMessage(msg);
    }

    return 0;
}