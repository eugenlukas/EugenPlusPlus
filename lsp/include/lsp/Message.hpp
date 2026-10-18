#pragma once

#define Request \
    std::string jsonrpc; \
    int id; \
    std::string method;

#define RequestArgs jsonrpc, id, method

#define Response \
    std::string jsonrpc; \
    int id;

#define ResponseArgs jsonrpc, id

#define Notification \
    std::string jsonrpc; \
    std::string method;

#define NotificationArgs jsonrpc, method