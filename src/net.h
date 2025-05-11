/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
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
