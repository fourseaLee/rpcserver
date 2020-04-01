// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 Bitcoin Association
// Distributed under the Open BSV software license, see the accompanying file LICENSE.

#ifndef BITCOIN_RPCCLIENT_H
#define BITCOIN_RPCCLIENT_H

#include "json.hpp"
#include <functional>

using json = nlohmann::json;
// Exit codes are EXIT_SUCCESS, EXIT_FAILURE, CONTINUE_EXECUTION 
static const int CONTINUE_EXECUTION = -1;  

json CallRPC(const std::string &strMethod, const json &params); 

//
// Exception thrown on connection error.  This error is used to determine when
// to wait if -rpcwait is given.
//
class CConnectionFailed : public std::runtime_error {
public:
    explicit inline CConnectionFailed(const std::string &msg)
        : std::runtime_error(msg) {}
};

struct HTTPReply 
{
    HTTPReply() : status(0), error(-1) {}

    int status;
    int error;
    std::string body;
};

static const char DEFAULT_RPCCONNECT[] = "127.0.0.1";
static const int DEFAULT_HTTP_CLIENT_TIMEOUT = 900;
static const bool DEFAULT_NAMED = false;

#endif // BITCOIN_RPCCLIENT_H
