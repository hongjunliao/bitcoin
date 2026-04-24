/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
#include <boost/test/unit_test.hpp>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <arpa/inet.h>
#include <openssl/sha.h> // For SHA-256 checksum
#include "btc_inc.h"
#include "btc_protocol.h"
#include "btc_node.h"

#define return_(code) do{ rc = code; goto exit_; } while(0)

static void compute_checksum(const uint8_t *data, size_t len, uint8_t *checksum);

// Compute SHA-256 checksum (first 4 bytes of double SHA-256)
static void compute_checksum(const uint8_t *data, size_t len, uint8_t *checksum) {
    uint8_t hash1[SHA256_DIGEST_LENGTH];
    uint8_t hash2[SHA256_DIGEST_LENGTH];
    SHA256(data, len, hash1);
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);
    memcpy(checksum, hash2, 4);
}

sds btc_p2pmsg_newc(const char *command, void * inpl, btc_p2p_hdr * outhdr, btc_p2p_payload * outpl)
{
	btc_p2p_hdr hdr{0};
	strcpy(hdr.command, command);
	return btc_p2pmsg_new(&hdr, 0, outhdr, outpl);
}

sds btc_p2pmsg_new(const btc_p2p_hdr *inhdr, btc_p2p_payload const * inpl, btc_p2p_hdr * outhdr, btc_p2p_payload * outpl)
{
	if(!(inhdr)) return 0;

	sds ret = sdsnewlen(0, BTC_HDR_SIZE);
	btc_p2p_hdr header = { 0 };
    memcpy(header.magic, "\xf9\xbe\xb4\xd9", 4); // Mainnet magic
    strcpy(header.command, inhdr->command);

    if(strncmpl(inhdr->command, "version") == 0){
		size_t len = 0;
		// Protocol version
		// NODE_NETWORK | NODE_WITNESS
    	btc_p2p_payload::VERSION version = { .version = 70016, .services = 1ULL | (1ULL << 3), .timestamp = time(NULL),
								.addr_recv_IP_address = ":ffff:127.0.0.1", .addr_recv_port = 8333, .addr_trans_services = 1ULL | (1ULL << 3),
								.addr_trans_IP_address = ":ffff:127.0.0.1", .addr_trans_port = 5555,
								.start_height = 3},
								* msg = inpl? (btc_p2p_payload::VERSION *)inpl : &version;
		size_t n[] = { sizeof(msg->version), sizeof(msg->services), sizeof(msg->timestamp), sizeof(msg->addr_recv_services),
				sizeof(msg->addr_recv_IP_address), sizeof(msg->addr_recv_port), sizeof(msg->addr_trans_services), sizeof(msg->addr_trans_IP_address),
				sizeof(msg->addr_trans_port), sizeof(msg->nonce), sizeof(msg->user_agent_bytes), sizeof(msg->start_height)
		};
	    void const * from[] = { &(msg->version), &(msg->services), &(msg->timestamp), &(msg->addr_recv_services),
				&(msg->addr_recv_IP_address), &(msg->addr_recv_port), &(msg->addr_trans_services), &(msg->addr_trans_IP_address),
				&(msg->addr_trans_port), &(msg->nonce), &(msg->user_agent_bytes), &(msg->start_height)
		};
		for(int i = 0; i < sizeof(n)/sizeof(n[0]); ++i){
			len += n[i];
			ret = sdscatlen(ret, from[i], n[i]);
		}
		header.length = len;

		if(outpl) outpl->version = version;
	}
	else if(strncmpl(inhdr->command, "ping") == 0){
		assert(inpl);
		btc_p2p_payload::PONG pong{.c = inpl->pong.c};
		ret = sdscatlen(ret, &pong.c, sizeof(pong.c));
		header.length = sizeof(pong.c);
	    strcpy(header.command, "pong");
	}
	else if(strncmpl(inhdr->command, "verack") == 0){
		header.length = 0;
	}
	else if(strncmpl(inhdr->command, "sendcmpct") == 0){
		btc_p2p_payload::SENDCMPCT sendcmpct{.c = ""};
//		ret = sdscatlen(ret, &sendcmpct.c, sizeof(sendcmpct));
	}
	else if(strncmpl(inhdr->command, "wtxidrelay") == 0){
	}
	else if(strncmpl(inhdr->command, "sendaddrv2") == 0){
	}
	else if(strncmpl(inhdr->command, "getheaders") == 0){
	}
	else if(strncmpl(inhdr->command, "feefilter") == 0){
	}

	compute_checksum((uint8_t *)ret + BTC_HDR_SIZE, header.length, header.checksum);
    if(outhdr) *outhdr = header;
    memcpy(ret, &header, BTC_HDR_SIZE);

    return ret;
}

int btc_p2pmsg_parse(char const * buf, size_t  len, btc_p2p_hdr * hdr, btc_p2p_payload * pl)
{
	if(!(buf && len > 0 && hdr && pl)) return 1;
	//消息长度至少达到BTC_HDR_SIZE,否则不是一个完整的bitcoin 消息
	if(!(len >= BTC_HDR_SIZE)) { return(2); }

	// Read header
	memcpy(hdr, buf, BTC_HDR_SIZE);

	//magic要匹配，否则不是bitcoin 消息(比如是垃圾数据)
	if(!(memcmp(hdr->magic, "\xf9\xbe\xb4\xd9", 4) == 0)){
		return(3);
	}
	//匹配到了一个合法bitcoin消息，检查消息的playload部分是否已同时到达
	if(len - BTC_HDR_SIZE < hdr->length){
		return(4);
	}
	// now parse payload
	if(strncmpl(hdr->command, "ping") == 0){
		if(hdr->length == sizeof(btc_p2p_payload::PONG::c)){
			memcpy(&pl->pong.c, buf + BTC_HDR_SIZE, sizeof(pl->pong.c));
		}
		else hp_log(stdout, "%s: NOT a valid message=ping, payload_len(received/expected)=%u/%u\n", __FUNCTION__
				, hdr->length, sizeof(btc_p2p_payload::PONG::c));
	}

	return 0;
}

BOOST_AUTO_TEST_SUITE()
BOOST_AUTO_TEST_CASE(node)
{
	btc_p2pmsg_parse(0, 0, 0, 0);
}
BOOST_AUTO_TEST_SUITE_END()

