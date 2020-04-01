// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPCSERVER_H
#define BITCOIN_RPCSERVER_H

#include "protocol.h"
#include <list>
#include <map>
#include <stdint.h>
#include <string>
#include "json.hpp"

static const unsigned int DEFAULT_RPC_SERIALIZE_VERSION = 1;

class CRPCCommand;

namespace RPCServer
{
    void OnStarted(std::function<void ()> slot);
    void OnStopped(std::function<void ()> slot);
}

/** Wrapper for json, which includes typeAny:
 * Used to denote don't care type. Only used by RPCTypeCheckObj */
struct jsonType {
    explicit jsonType(json::value_t  _type) : typeAny(false), type(_type) {}
    jsonType() : typeAny(true) {}
    bool typeAny;
    json::value_t type;
};

class JSONRPCRequest
{
public:
    json id;
    std::string strMethod;
    json params;
    bool fHelp;
    std::string URI;
    std::string authUser;

    JSONRPCRequest() : id(json::object()), params(json::object()), fHelp(false) {}
    void parse(const json& valRequest);
};

/** Query whether RPC is running */
bool IsRPCRunning();

/**
 * Set the RPC warmup status.  When this is done, all RPC calls will error out
 * immediately with RPC_IN_WARMUP.
 */
void SetRPCWarmupStatus(const std::string& newStatus);
/* Mark warmup as done.  RPC calls will be processed from now on.  */
void SetRPCWarmupFinished();

/* returns the current warmup state.  */
bool RPCIsInWarmup(std::string *outStatus);

/**
 * Type-check arguments; throws JSONRPCError if wrong type given. Does not check that
 * the right number of arguments are passed, just that any passed are the correct type.
 */
void RPCTypeCheck(const json& params,
                  const std::list<json::value_t>& typesExpected, bool fAllowNull=false);

/**
 * Type-check one argument; throws JSONRPCError if wrong type given.
 */
void RPCTypeCheckArgument(const json& value, json::value_t typeExpected);

/*
  Check for expected keys/value types in an Object.
*/
void RPCTypeCheckObj(const json& o,
    const std::map<std::string, jsonType>& typesExpected,
    bool fAllowNull = false,
    bool fStrict = false);

/** Opaque base class for timers returned by NewTimerFunc.
 * This provides no methods at the moment, but makes sure that delete
 * cleans up the whole state.
 */
class RPCTimerBase
{
public:
    virtual ~RPCTimerBase() {}
};

/**
 * RPC timer "driver".
 */
class RPCTimerInterface
{
public:
    virtual ~RPCTimerInterface() {}
    /** Implementation name */
    virtual const char *Name() = 0;
    /** Factory function for timers.
     * RPC will call the function to create a timer that will call func in *millis* milliseconds.
     * @note As the RPC mechanism is backend-neutral, it can use different implementations of timers.
     * This is needed to cope with the case in which there is no HTTP server, but
     * only GUI RPC console, and to break the dependency of pcserver on httprpc.
     */
    virtual RPCTimerBase* NewTimer(std::function<void(void)>& func, int64_t millis) = 0;
};

/** Set the factory function for timers */
void RPCSetTimerInterface(RPCTimerInterface *iface);
/** Set the factory function for timer, but only, if unset */
void RPCSetTimerInterfaceIfUnset(RPCTimerInterface *iface);
/** Unset factory function for timers */
void RPCUnsetTimerInterface(RPCTimerInterface *iface);

/**
 * Run func nSeconds from now.
 * Overrides previous timer <name> (if any).
 */
void RPCRunLater(const std::string& name, std::function<void(void)> func, int64_t nSeconds);

typedef json(*rpcfn_type)(const JSONRPCRequest& jsonRequest);

class CRPCCommand
{
public:
    std::string category;
    std::string name;
    rpcfn_type actor;
    std::vector<std::string> argNames;
};

/**
 * Bitcoin RPC command dispatcher.
 */
class CRPCTable
{
private:
    std::map<std::string, const CRPCCommand*> mapCommands;
public:
    CRPCTable();
    const CRPCCommand* operator[](const std::string& name) const;
    std::string help(const std::string& name, const JSONRPCRequest& helpreq) const;

    /**
     * Execute a method.
     * @param request The JSONRPCRequest to execute
     * @returns Result of the call.
     * @throws an exception (json) when an error happens.
     */
    json execute(const JSONRPCRequest &request) const;

    /**
    * Returns a list of registered commands
    * @returns List of registered commands.
    */
    std::vector<std::string> listCommands() const;


    /**
     * Appends a CRPCCommand to the dispatch table.
     * Returns false if RPC server is already running (dump concurrency protection).
     * Commands cannot be overwritten (returns false).
     */
    bool appendCommand(const std::string& name, const CRPCCommand* pcmd);
};

bool IsDeprecatedRPCEnabled(const std::string& method);

extern CRPCTable tableRPC;

extern std::vector<std::string> vectFileSendTx;
/**
 * Utilities: convert hex-encoded Values
 * (throws error if not hex).
 */
//extern uint256 ParseHashV(const json& v, std::string strName);
//extern uint256 ParseHashO(const json& o, std::string strKey);
//extern std::vector<unsigned char> ParseHexV(const json& v, std::string strName);
//extern std::vector<unsigned char> ParseHexO(const json& o, std::string strKey);

//extern CAmount AmountFromValue(const json& value);
extern std::string HelpExampleCli(const std::string& methodname, const std::string& args);
extern std::string HelpExampleRpc(const std::string& methodname, const std::string& args);

bool StartRPC();
void InterruptRPC();
void StopRPC();
std::string JSONRPCExecBatch(const JSONRPCRequest& jreq, const json& vReq);

// Retrieves any serialization flags requested in command line argument
int RPCSerializationFlags();

#endif // BITCOIN_RPCSERVER_H
