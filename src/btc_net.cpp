/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
#include "btc_net.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <boost/test/unit_test.hpp>
#include "hp/hp_log.h"

// dnsLookup 函数：查询 DNS 种子并返回解析出的 IP 地址列表
// 参数:
//   - hostname: DNS 种子地址（例如 "seed.bitcoin.sipa.be"）
//   - port: Bitcoin 网络端口（主网默认 8333）
// 返回:
//   - std::vector<addrinfo: 解析出的 IP 地址列表（IPv4 或 IPv6）
std::vector<struct addrinfo> dnsLookup(const std::string& hostname, const std::string& port)
{
    std::vector<struct addrinfo> ip_addresses;

    // 准备 getaddrinfo 所需的结构体
    struct addrinfo hints;
    struct addrinfo* result = nullptr;

    // 清空 hints 结构体
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // 支持 IPv4 和 IPv6
    hints.ai_socktype = SOCK_STREAM; // 使用 TCP 流
    hints.ai_protocol = IPPROTO_TCP; // 协议为 TCP

    // 调用 getaddrinfo 进行 DNS 解析
    int status = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    if (status != 0) {
        hp_log(std::cerr, "DNS lookup failed for '%s': '%s'\n", hostname, gai_strerror(status));
        return ip_addresses;
    }

    // 遍历解析结果
    for (struct addrinfo* p = result; p != nullptr; p = p->ai_next) {
        // 将解析出的 IP 地址添加到结果列表
        ip_addresses.emplace_back(*p);
    }

    // 释放 getaddrinfo 分配的内存
    freeaddrinfo(result);

    return ip_addresses;
}

BOOST_AUTO_TEST_SUITE(net_tests)

BOOST_AUTO_TEST_CASE(dns_lookup_test) {
	return;
    std::vector<std::string> vSeeds;
//	vSeeds.emplace_back("seed.bitcoin.sipa.be."); // Pieter Wuille, only supports x1, x5, x9, and xd
	vSeeds.emplace_back("dnsseed.bluematt.me."); // Matt Corallo, only supports x9
	vSeeds.emplace_back("dnsseed.bitcoin.dashjr-list-of-p2p-nodes.us."); // Luke Dashjr
//	vSeeds.emplace_back("seed.bitcoin.jonasschnelli.ch."); // Jonas Schnelli, only supports x1, x5, x9, and xd
	vSeeds.emplace_back("seed.btc.petertodd.net."); // Peter Todd, only supports x1, x5, x9, and xd
	vSeeds.emplace_back("seed.bitcoin.sprovoost.nl."); // Sjors Provoost
	vSeeds.emplace_back("dnsseed.emzy.de."); // Stephan Oeste
	vSeeds.emplace_back("seed.bitcoin.wiz.biz."); // Jason Maurice
	vSeeds.emplace_back("seed.mainnet.achownodes.xyz."); // Ava Chow, only supports x1, x5, x9, x49, x809, x849, xd, x400, x404, x408, x448, xc08, xc48, x40c
	std::vector<struct addrinfo> ips;
	for(auto const& seed : vSeeds){
		 auto && ip = dnsLookup(seed, "8333");
		 ips.insert(ips.end(), ip.begin(), ip.end());
	}
	BOOST_CHECK(!ips.empty());
	hp_log(std::cout, "total=%d\n", ips.size());
	for(auto & p : ips){
        char ip_str[INET6_ADDRSTRLEN]; // 足够存储 IPv4 或 IPv6 地址

        // 根据地址族（IPv4 或 IPv6）提取 IP 地址
        if (p.ai_family == AF_INET) { // IPv4
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p.ai_addr;
            inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, sizeof(ip_str));
        } else if (p.ai_family == AF_INET6) { // IPv6
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p.ai_addr;
            inet_ntop(AF_INET6, &(ipv6->sin6_addr), ip_str, sizeof(ip_str));
        } else {
            continue; // 忽略不支持的地址族
        }

		hp_log(std::cout, "%s\n", ip_str); }
}

BOOST_AUTO_TEST_SUITE_END()
