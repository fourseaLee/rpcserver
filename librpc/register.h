// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPCREGISTER_H
#define BITCOIN_RPCREGISTER_H

/** These are in one header file to avoid creating tons of single-function
 * headers for everything under src/rpc/ */
class CRPCTable;

template<class T>
static inline void RegisterAllCoreRPCCommands(T& regist,CRPCTable &t)
{
   regist.RegisterRPRCCommands(t);
}
#endif
