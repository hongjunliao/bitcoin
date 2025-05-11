// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BTC_NET_H
#define BTC_NET_H
#include <netdb.h>
#include <string>
#include <vector>
class CMessageHeader {

};
typedef struct CDataStream {

} CDataStream;
typedef struct btc_msg{
	CDataStream    ds;
} btc_msg;
std::vector<addrinfo> dnsLookup(const std::string& hostname, const std::string& port);
#endif
