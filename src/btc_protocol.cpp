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
#include <stdint.h>

#define return_(code) do{ rc = code; goto exit_; } while(0)
static void compute_checksum(const uint8_t *data, size_t len, uint8_t *checksum);

#define memcpy_(t,buf)  do { memcpy(&(t), (buf), sizeof(t)); }while(0)
/**!
 * deseiralize
 */
static int des_cmpctsize(uint32_t & t, const char *buf);
#define des_szof(t,buf)  do { memcpy(&(t), (buf), sizeof(t)); (buf) += sizeof(t); }while(0)
#define des_sz(t,buf,l,r)  do { memcpy(&(t), (buf), hp_min((l),(r))); (buf) += (r); }while(0)
#define des_cmpctsz(t, buf)  do { int des_cmpctsz_ret = des_cmpctsize((t), (buf)); if( des_cmpctsz_ret > 0) (buf) += des_cmpctsz_ret; }while(0)
/**!
 * @return: actual bytes of @param t
 */
static int des_cmpctsize(uint32_t & t, const char *buf)
{
    uint8_t ch1 = 0;uint16_t ch2 = 0;uint32_t ch4 = 0;
    memcpy_(ch1, buf);

    t = 0;
    if (ch1 < 253)
    {
        t = ch1;
        return 1;
    }
    else if (ch1 == 253)
    {
    	memcpy_(ch2, buf);
        t = ch2;
        if (t < 253)
            return 0;
        return 2;
    }
    else if (ch1 == 254)
    {
    	memcpy_(ch4, buf);
        t = ch4;
        if (t < 0x10000u)
            return 0;
        return 4;
    }
    else
    {
    	memcpy_(t, buf);
        if (t < 0x100000000ULL)
            return 0;
        return 8;
    }
    if (t > MAX_SIZE) {
        return 0;
    }
    return 0;
}

// Compute SHA-256 checksum (first 4 bytes of double SHA-256)
static void compute_checksum(const uint8_t *data, size_t len, uint8_t *checksum) {
    uint8_t hash1[SHA256_DIGEST_LENGTH];
    uint8_t hash2[SHA256_DIGEST_LENGTH];
    SHA256(data, len, hash1);
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);
    memcpy(checksum, hash2, 4);
}

sds btc_p2pmsg_reply(const btc_p2p_hdr *inhdr, btc_p2p_payload const * inpl, btc_p2p_hdr * outhdr, btc_p2p_payload * outpl)
{
	if(!(outhdr)) return 0;

	memset(outhdr, 0, sizeof(*outhdr));
    memcpy(outhdr->magic, "\xf9\xbe\xb4\xd9", 4); // Mainnet magic
	sds ret = sdsnewlen(0, BTC_HDR_SIZE);

    if(!inhdr){	//version
        strcpy(outhdr->command, "version");
		size_t len = 0;
		// Protocol version
		// NODE_NETWORK | NODE_WITNESS
    	btc_p2p_payload::VERSION version = {
    			.version = 70016, .services = 1ULL | (1ULL << 3), .timestamp = time(NULL), .addr_recv_services = 1,
				.addr_recv_IP_address = "127.0.0.1", .addr_recv_port = 8333, .addr_trans_services = 1ULL | (1ULL << 3),.addr_trans_IP_address = "127.0.0.1",
				.addr_trans_port = 5555, .nonce = 0, .user_agent_bytes = 0, /*.user_agent = "btcc",*/
				.start_height = 3/*, .relay = 0*/ }, * pl = &version;
		size_t n[] = {
				sizeof(pl->version), sizeof(pl->services), sizeof(pl->timestamp), sizeof(pl->addr_recv_services),
				sizeof(pl->addr_recv_IP_address), sizeof(pl->addr_recv_port), sizeof(pl->addr_trans_services), sizeof(pl->addr_trans_IP_address),
				sizeof(pl->addr_trans_port), sizeof(pl->nonce), sizeof(pl->user_agent_bytes), /*sizeof(pl->user_agent),*/
				sizeof(pl->start_height)/*, sizeof(pl->relay)*/
		};
	    void const * from[] = {
	    		&(pl->version), &(pl->services), &(pl->timestamp), &(pl->addr_recv_services),
				(pl->addr_recv_IP_address), &(pl->addr_recv_port), &(pl->addr_trans_services), (pl->addr_trans_IP_address),
				&(pl->addr_trans_port), &(pl->nonce), &(pl->user_agent_bytes), /*pl->user_agent,*/
				&(pl->start_height)/*, &(pl->relay)*/
		};
		for(int i = 0; i < sizeof(n)/sizeof(n[0]); ++i){
			len += n[i];
			ret = sdscatlen(ret, from[i], n[i]);
		}
		outhdr->length = len;

		if(outpl) outpl->version = version;
	}
	else if(strncmpl(inhdr->command, "ping") == 0){
		assert(inpl);
	    strcpy(outhdr->command, "pong");
		btc_p2p_payload::PONG pong{.c = inpl->pong.c};
		ret = sdscatlen(ret, &pong.c, sizeof(pong.c));
		outhdr->length = sizeof(pong.c);
		if(outpl) outpl->pong = pong;
	}
	else if(strncmpl(inhdr->command, "version") == 0 || strncmpl(inhdr->command, "verack") == 0){
	    strcpy(outhdr->command, "verack");
	}
	else if(strncmpl(inhdr->command, "sendcmpct") == 0){
		btc_p2p_payload::CMPCTBLOCK cmpctblock;
		ret = sdscatlen(ret, &cmpctblock, sizeof(cmpctblock));
		outhdr->length = sizeof(cmpctblock);
		if(outpl) outpl->cmpctblock = cmpctblock;
	}
	else if(strncmpl(inhdr->command, "wtxidrelay") == 0){
	}
	else if(strncmpl(inhdr->command, "sendaddrv2") == 0){
	}
	else if(strncmpl(inhdr->command, "getheaders") == 0){
	    strcpy(outhdr->command, "headers");
		btc_p2p_payload::HEADERS headers{.count = 0};
		ret = sdscatlen(ret, &headers, sizeof(headers));
		outhdr->length = sizeof(headers);
		if(outpl) outpl->headers = headers;
	}
	else if(strncmpl(inhdr->command, "feefilter") == 0){
	}

    if(outhdr->command[0] == '\0'){
	    strcpy(outhdr->command, "unkown");
	    outhdr->length = 0;
	}

	compute_checksum((uint8_t *)ret + BTC_HDR_SIZE, outhdr->length, outhdr->checksum);
    memcpy(ret, outhdr, BTC_HDR_SIZE);

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
	else if(strncmpl(hdr->command, "version") == 0){
		//4+8+8+8+16+2+8+16+2+8+[1-4]+32+4+1
		if(hdr->length >= 81){
			auto p = buf + BTC_HDR_SIZE;
			des_szof(pl->version.version, p);
			des_szof(pl->version.services, p);
			des_szof(pl->version.timestamp, p);
			des_szof(pl->version.addr_recv_services, p);
			des_szof(pl->version.addr_recv_IP_address, p);
			des_szof(pl->version.addr_recv_port, p);
			des_szof(pl->version.addr_trans_services, p);
			des_szof(pl->version.addr_trans_IP_address, p);
			des_szof(pl->version.addr_trans_port, p);
			des_szof(pl->version.nonce, p);
			des_cmpctsz(pl->version.user_agent_bytes, p);
			assert(p - (buf + BTC_HDR_SIZE) >= 81);

			if(pl->version.user_agent_bytes <= hdr->length - (p - (buf + BTC_HDR_SIZE))){
				des_sz(pl->version.user_agent, p, sizeof(pl->version.user_agent), pl->version.user_agent_bytes);
			}
			if(hdr->length - (p - (buf + BTC_HDR_SIZE)) >= 4){
				des_szof(pl->version.start_height, p);
			}
			if(hdr->length - (p - (buf + BTC_HDR_SIZE)) > 0){
				des_szof(pl->version.relay, p);
			}
			if(hdr->length - (p - (buf + BTC_HDR_SIZE)) > 0){
				hp_log(stdout, "%s: ignored redundant data; message=version, length=%u\n", __FUNCTION__
						, hdr->length - (p - (buf + BTC_HDR_SIZE)));
			}
		}
		else hp_log(stdout, "%s: NOT a valid message=version, payload_len(received/expected)=%u/%u(min)\n", __FUNCTION__
				, hdr->length, 81);
	}


	return 0;
}

BOOST_AUTO_TEST_SUITE()
BOOST_AUTO_TEST_CASE(node)
{
	btc_p2pmsg_parse(0, 0, 0, 0);
}
BOOST_AUTO_TEST_SUITE_END()

