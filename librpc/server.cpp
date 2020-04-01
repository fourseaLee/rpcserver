// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "server.h"
#include <set>

#include <boost/bind.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/algorithm/string/case_conv.hpp> // for to_upper()
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <memory> // for unique_ptr
#include <unordered_map>

static bool fRPCRunning = false;
static bool fRPCInWarmup = true;
static std::string rpcWarmupStatus("RPC server started");
//static CCriticalSection cs_rpcWarmup;
/* Timer-creating functions */
static RPCTimerInterface* timerInterface = nullptr;
/* Map of name to timer. */
static std::map<std::string, std::unique_ptr<RPCTimerBase> > deadlineTimers;

static struct CRPCSignals
{
    boost::signals2::signal<void ()> Started;
    boost::signals2::signal<void ()> Stopped;
    boost::signals2::signal<void (const CRPCCommand&)> PreCommand;
} g_rpcSignals;

void RPCServer::OnStarted(std::function<void ()> slot)
{
    g_rpcSignals.Started.connect(slot);
}

void RPCServer::OnStopped(std::function<void ()> slot)
{
    g_rpcSignals.Stopped.connect(slot);
}

void RPCTypeCheck(const json& params,
                  const std::list<json::value_t>& typesExpected,
                  bool fAllowNull)
{
    unsigned int i = 0;
    for (json t : typesExpected)
    {
        if (params.size() <= i)
            break;

        const json& v = params[i];
        if (!(fAllowNull && v.is_null())) {
            RPCTypeCheckArgument(v, t);
        }
        i++;
    }
}

void RPCTypeCheckArgument(const json& value, json::value_t typeExpected)
{
    if (value.type() != typeExpected) {
        //std::runtime_error("json type error"); 
        //throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Expected type , got %s",value.type_name()));
        //throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Expected type %s, got %s", uvTypeName(typeExpected), uvTypeName(value.type())));
    }
}

void RPCTypeCheckObj(const json& o,
    const std::map<std::string, jsonType>& typesExpected,
    bool fAllowNull,
    bool fStrict)
{
    for (const auto& t : typesExpected) {
        const json& v =  o[t.first]; //find_value(o, t.first);
        if (!fAllowNull && v.is_null())
        {
            //throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Missing %s", t.first));
        }
        //if (!fAllowNull && v.is_null())
        //    std::runtime_error("json type error"); 
           // throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Missing %s", t.first));
     /*       
        if (!(t.second.typeAny || v.type() == t.second.type || (fAllowNull && v.isNull()))) {
            std::string err = strprintf("Expected type %s for %s, got %s",
                uvTypeName(t.second.type), t.first, uvTypeName(v.type()));
            throw std::runtime_error("json type error");
            //throw JSONRPCError(RPC_TYPE_ERROR, err);
        }*/

        if (!(t.second.typeAny || v.type() == t.second.type || (fAllowNull && v.is_null())))
        {
            //std::string err = strprintf("Expected type s for %s, got %s",t.first, v.type_name());
            //throw JSONRPCError(RPC_TYPE_ERROR, err);
        }
    }

    if (fStrict)
    {

        json::const_iterator iter = o.begin();
        for(;iter != o.end();iter++)
        {
            std::string key = iter.key();
            if (typesExpected.count(key) == 0)
            {
                //std::string err = strprintf("Unexpected key %s",key);
                //throw  JSONRPCError(RPC_TYPE_ERROR,err);
            }
        }

        /*for (const std::string& k : o.getKeys())
        {
            if (typesExpected.count(k) == 0)
            {
                std::string err = strprintf("Unexpected key %s", k);
                throw  std::runtime_error("json type error");
                //throw JSONRPCError(RPC_TYPE_ERROR, err);
            }
        }*/
    }
}
/*
CAmount AmountFromValue(const json& value)
{
    if (!value.isNum() && !value.isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Amount is not a number or string");
    CAmount amount;
    if (!ParseFixedPoint(value.getValStr(), 8, &amount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    if (!MoneyRange(amount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Amount out of range");
    return amount;
}
*/
/*uint256 ParseHashV(const json& v, std::string strName)
{
    std::string strHex;
    if (v.is_string())
        strHex = v;
    if (!IsHex(strHex)) // Note: IsHex("") is false
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    if (64 != strHex.length())
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be of length %d (not %d)", strName, 64, strHex.length()));
    uint256 result;
    result.SetHex(strHex);
    return result;
}
uint256 ParseHashO(const json& o, std::string strKey)
{
    json json_value = o[strKey];
    return ParseHashV(json_value, strKey);
    //return ParseHashV(find_value(o, strKey), strKey);
}
std::vector<unsigned char> ParseHexV(const json& v, std::string strName)
{
    std::string strHex;
    if (v.is_string())
        strHex = v;
    if (!IsHex(strHex))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    return ParseHex(strHex);
}
std::vector<unsigned char> ParseHexO(const json& o, std::string strKey)
{
    json o_value = o[strKey];
    return ParseHexV(find_value(o, strKey) o_value, strKey);
}
*/
/**
 * Note: This interface may still be subject to change.
 */

std::string CRPCTable::help(const std::string& strCommand, const JSONRPCRequest& helpreq) const
{
    std::string strRet;
    std::string category;
    std::set<rpcfn_type> setDone;
    std::vector<std::pair<std::string, const CRPCCommand*> > vCommands;

    for (const auto& entry : mapCommands)
        vCommands.push_back(make_pair(entry.second->category + entry.first, entry.second));
    sort(vCommands.begin(), vCommands.end());

    JSONRPCRequest jreq(helpreq);
    jreq.fHelp = true;
    jreq.params = json();

    for (const std::pair<std::string, const CRPCCommand*>& command : vCommands)
    {
        const CRPCCommand *pcmd = command.second;
        std::string strMethod = pcmd->name;
        if ((strCommand != "" || pcmd->category == "hidden") && strMethod != strCommand)
            continue;
        jreq.strMethod = strMethod;
        try
        {
            rpcfn_type pfn = pcmd->actor;
            if (setDone.insert(pfn).second)
                (*pfn)(jreq);
        }
        catch (const std::exception& e)
        {
            // Help text is returned in an exception
            std::string strHelp = std::string(e.what());
            if (strCommand == "")
            {
                if (strHelp.find('\n') != std::string::npos)
                    strHelp = strHelp.substr(0, strHelp.find('\n'));

                if (category != pcmd->category)
                {
                    if (!category.empty())
                        strRet += "\n";
                    category = pcmd->category;
                    std::string firstLetter = category.substr(0,1);
                    boost::to_upper(firstLetter);
                    strRet += "== " + firstLetter + category.substr(1) + " ==\n";
                }
            }
            strRet += strHelp + "\n";
        }
    }
    //if (strRet == "")
    //    strRet = strprintf("help: unknown command: %s\n", strCommand);
    strRet = strRet.substr(0,strRet.size()-1);
    return strRet;
}

json help(const JSONRPCRequest& jsonRequest)
{
    if (jsonRequest.fHelp || jsonRequest.params.size() > 1)
        throw std::runtime_error(
            "help ( \"command\" )\n"
            "\nList all commands, or get help for a specified command.\n"
            "\nArguments:\n"
            "1. \"command\"     (string, optional) The command to get help on\n"
            "\nResult:\n"
            "\"text\"     (string) The help text\n"
        );

    std::string strCommand;
    if (jsonRequest.params.size() > 0)
        strCommand = jsonRequest.params[0].get<std::string>();

    return tableRPC.help(strCommand, jsonRequest);
}


json stop(const JSONRPCRequest& jsonRequest)
{
    // Accept the deprecated and ignored 'detach' boolean argument
    if (jsonRequest.fHelp || jsonRequest.params.size() > 1)
        throw std::runtime_error(
            "stop\n"
            "\nStop Bitcoin server.");
    // Event loop will exit after current HTTP requests have been handled, so
    // this reply will get back to the client.
    //StartShutdown();
    return "Bitcoin server stopping";
}

json uptime(const JSONRPCRequest& jsonRequest)
{
    if (jsonRequest.fHelp || jsonRequest.params.size() > 1)
        throw std::runtime_error(
                "uptime\n"
                        "\nReturns the total uptime of the server.\n"
                        "\nResult:\n"
                        "ttt        (numeric) The number of seconds that the server has been running\n"
                        "\nExamples:\n"
                + HelpExampleCli("uptime", "")
                + HelpExampleRpc("uptime", "")
        );

    return 0;//GetTime() - GetStartupTime();
}

/**
 * Call Table
 */
static const CRPCCommand vRPCCommands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    /* Overall control/query calls */
    { "control",            "help",                   &help,                   {"command"}  },
    { "control",            "stop",                   &stop,                   {}  },
    { "control",            "uptime",                 &uptime,                 {}  },
};

CRPCTable::CRPCTable()
{
    unsigned int vcidx;
    for (vcidx = 0; vcidx < (sizeof(vRPCCommands) / sizeof(vRPCCommands[0])); vcidx++)
    {
        const CRPCCommand *pcmd;

        pcmd = &vRPCCommands[vcidx];
        mapCommands[pcmd->name] = pcmd;
    }
}

const CRPCCommand *CRPCTable::operator[](const std::string &name) const
{
    std::map<std::string, const CRPCCommand*>::const_iterator it = mapCommands.find(name);
    if (it == mapCommands.end())
        return nullptr;
    return (*it).second;
}

bool CRPCTable::appendCommand(const std::string& name, const CRPCCommand* pcmd)
{
    if (IsRPCRunning())
        return false;

    // don't allow overwriting for now
    std::map<std::string, const CRPCCommand*>::const_iterator it = mapCommands.find(name);
    if (it != mapCommands.end())
        return false;

    mapCommands[name] = pcmd;
    return true;
}

bool StartRPC()
{
    //LogPrint(BCLog::RPC, "Starting RPC\n");
    fRPCRunning = true;
    g_rpcSignals.Started();
    return true;
}

void InterruptRPC()
{
    //LogPrint(BCLog::RPC, "Interrupting RPC\n");
    // Interrupt e.g. running longpolls
    fRPCRunning = false;
}

void StopRPC()
{
    //LogPrint(BCLog::RPC, "Stopping RPC\n");
    deadlineTimers.clear();
    DeleteAuthCookie();
    g_rpcSignals.Stopped();
}

bool IsRPCRunning()
{
    return fRPCRunning;
}

void SetRPCWarmupStatus(const std::string& newStatus)
{
    //LOCK(cs_rpcWarmup);
    rpcWarmupStatus = newStatus;
}

void SetRPCWarmupFinished()
{
    //LOCK(cs_rpcWarmup);GetDataDir
    assert(fRPCInWarmup);
    fRPCInWarmup = false;
}

bool RPCIsInWarmup(std::string *outStatus)
{
    //LOCK(cs_rpcWarmup);
    if (outStatus)
        *outStatus = rpcWarmupStatus;
    return fRPCInWarmup;
}

void JSONRPCRequest::parse(const json& valRequest)
{
    // Parse requestGetDataDirGetDataDir
//    ifGetDataDirGetDataDirGetDataDir (!valRequest.is_object())
//        throw JSONRPCError(RPC_INVALID_REQUEST, "Invalid Request object");
    const json& request  = valRequest;//= valRequest.get_obj();

    // Parse id now so errors from here on will have the id
    id = request["id"];//find_value(request, "id");

    // Parse method
    json valMethod = request["method"];//find_value(request, "method");
    if (valMethod.is_null())
        throw JSONRPCError(RPC_INVALID_REQUEST, "Missing method");
    if (!valMethod.is_string())
        throw JSONRPCError(RPC_INVALID_REQUEST, "Method must be a string");
    strMethod = request["method"].get<std::string>();
    //LogPrint(BCLog::RPC, "ThreadRPCServer method=%s\n", SanitizeString(strMethod));

    // Parse params
    json valParams = request["params"];//find_value(request, "params");
    if (valParams.is_array() || valParams.is_object())
        params = valParams;
    else if (valParams.is_null())
        params.clear(); //= json::json_array();
    else
        throw JSONRPCError(RPC_INVALID_REQUEST, "Params must be an array or object");
}

bool IsDeprecatedRPCEnabled(const std::string& method)
{
    const std::vector<std::string> enabled_methods ;//= gArgs.GetArgs("-deprecatedrpc");

    return find(enabled_methods.begin(), enabled_methods.end(), method) != enabled_methods.end();
}

static json JSONRPCExecOne(JSONRPCRequest jreq, const json& req)
{
    json rpc_result=json::object();
    json Nulljson;
    try {
        jreq.parse(req);

        json result = tableRPC.execute(jreq);
        rpc_result = JSONRPCReplyObj(result, Nulljson, jreq.id);
    }
    catch (const json& objError)
    {
        rpc_result = JSONRPCReplyObj(Nulljson, objError, jreq.id);
    }
    catch (const std::exception& e)
    {
        rpc_result = JSONRPCReplyObj(Nulljson,
                                     JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
    }

    return rpc_result;
}

std::string JSONRPCExecBatch(const JSONRPCRequest& jreq, const json& vReq)
{
    json ret=json::array();
    for (unsigned int reqIdx = 0; reqIdx < vReq.size(); reqIdx++)
        ret.push_back(JSONRPCExecOne(jreq, vReq[reqIdx]));

    return ret.dump() + "\n";
}

/**
 * Process named arguments into a vector of positional arguments, based on the
 * passed-in specification for the RPC call's arguments.
 */
static inline JSONRPCRequest transformNamedArguments(const JSONRPCRequest& in, const std::vector<std::string>& argNames)
{
    JSONRPCRequest out = in;
   // out.params = json::array();
    // Build a map of parameters, and remove ones that have been processed, so that we can throw a focused error if
    // there is an unknown one.
    const std::vector<std::string> keys;//& keys = in.params.getKeys();
    const std::vector<json> values; //& values = in.params.getValues();
    std::unordered_map<std::string, const json*> argsIn;
    for (size_t i=0; i<keys.size(); ++i) {
        argsIn[keys[i]] = &values[i];
    }
    // Process expected parameters.
    int hole = 0;
    for (const std::string &argNamePattern: argNames) {
        std::vector<std::string> vargNames;
        boost::algorithm::split(vargNames, argNamePattern, boost::algorithm::is_any_of("|"));
        auto fr = argsIn.end();
        for (const std::string & argName : vargNames) {
            fr = argsIn.find(argName);
            if (fr != argsIn.end()) {
                break;
            }
        }
        if (fr != argsIn.end()) {
            for (int i = 0; i < hole; ++i) {
                // Fill hole between specified parameters with JSON nulls,
                // but not at the end (for backwards compatibility with calls
                // that act based on number of specified parameters).
                out.params.push_back(json());
            }
            hole = 0;
            out.params.push_back(*fr->second);
            argsIn.erase(fr);
        } else {
            hole += 1;
        }
    }
    // If there are still arguments in the argsIn map, this is an error.
    if (!argsIn.empty()) {
        //throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown named parameter " + argsIn.begin()->first);
    }
    // Return request with named arguments transformed to positional arguments
    return out;
}

json CRPCTable::execute(const JSONRPCRequest &request) const
{
    // Return immediately if in warmup
    {
        //LOCK(cs_rpcWarmup);
        if (fRPCInWarmup)
            throw JSONRPCError(RPC_IN_WARMUP, rpcWarmupStatus);
    }

    // Find method
    const CRPCCommand *pcmd = tableRPC[request.strMethod];
    if (!pcmd)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found");

    g_rpcSignals.PreCommand(*pcmd);

    try
    {
        // Execute, convert arguments to array if necessary
        if (request.params.is_object()) {
            return pcmd->actor(transformNamedArguments(request, pcmd->argNames));
        } else {
            return pcmd->actor(request);
        }
    }
    catch (const std::exception& e)
    {
        throw JSONRPCError(RPC_MISC_ERROR, e.what());
    }
}

std::vector<std::string> CRPCTable::listCommands() const
{
    std::vector<std::string> commandList;
    typedef std::map<std::string, const CRPCCommand*> commandMap;

    std::transform( mapCommands.begin(), mapCommands.end(),
                   std::back_inserter(commandList),
                   boost::bind(&commandMap::value_type::first,_1) );
    return commandList;
}

std::string HelpExampleCli(const std::string& methodname, const std::string& args)
{
    return "> bitcoin-cli " + methodname + " " + args + "\n";
}

std::string HelpExampleRpc(const std::string& methodname, const std::string& args)
{
    return "> curl --user myusername --data-binary '{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", "
        "\"method\": \"" + methodname + "\", \"params\": [" + args + "] }' -H 'content-type: text/plain;' http://127.0.0.1:8332/\n";
}

void RPCSetTimerInterfaceIfUnset(RPCTimerInterface *iface)
{
    if (!timerInterface)
        timerInterface = iface;
}

void RPCSetTimerInterface(RPCTimerInterface *iface)
{
    timerInterface = iface;
}

void RPCUnsetTimerInterface(RPCTimerInterface *iface)
{
    if (timerInterface == iface)
        timerInterface = nullptr;
}

void RPCRunLater(const std::string& name, std::function<void(void)> func, int64_t nSeconds)
{
    if (!timerInterface)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No timer handler registered for RPC");
    deadlineTimers.erase(name);
    //LogPrint(BCLog::RPC, "queue run of timer %s in %i seconds (using %s)\n", name, nSeconds, timerInterface->Name());
    deadlineTimers.emplace(name, std::unique_ptr<RPCTimerBase>(timerInterface->NewTimer(func, nSeconds*1000)));
}

int RPCSerializationFlags()
{
    int flag = 0;
    //if (gArgs.GetArg("-rpcserialversion", DEFAULT_RPC_SERIALIZE_VERSION) == 0)
    //    flag |= 9;// SERIALIZE_TRANSACTION_NO_WITNESS;
    return flag;
}

CRPCTable tableRPC;
