/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
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
#include "btc_protocol.h"


static void compute_checksum(const uint8_t *data, size_t len, uint8_t *checksum);

// Compute SHA-256 checksum (first 4 bytes of double SHA-256)
static void compute_checksum(const uint8_t *data, size_t len, uint8_t *checksum) {
    uint8_t hash1[SHA256_DIGEST_LENGTH];
    uint8_t hash2[SHA256_DIGEST_LENGTH];
    SHA256(data, len, hash1);
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);
    memcpy(checksum, hash2, 4);
}

sds btc_p2pmsg_new(const char *command, void * payload, btc_p2p_hdr * hdr, btc_p2p_payload * pl)
{
	sds ret = sdsnewlen(0, BTC_HDR_SIZE);
	btc_p2p_hdr header = { 0 };
    memcpy(header.magic, "\xf9\xbe\xb4\xd9", 4); // Mainnet magic
    strcpy(header.command, command);

    if(strncmp(command, "version", 7) == 0){
		// Protocol version
		// NODE_NETWORK | NODE_WITNESS
		CVersionMsg version = { .version = 70016, .services = 1ULL | (1ULL << 10), .timestamp = time(NULL)},
								* msg = payload? (CVersionMsg *)payload : &version;
		size_t n[] = { sizeof(msg->version), sizeof(msg->services), sizeof(msg->timestamp), sizeof(msg->addr_recv_services),
				sizeof(msg->addr_recv_IP_address), sizeof(msg->addr_recv_port), sizeof(msg->addr_trans_services), sizeof(msg->addr_trans_IP_address),
				sizeof(msg->addr_trans_port), sizeof(msg->nonce), sizeof(msg->user_agent_bytes), sizeof(msg->start_height)
		};
	    void const * from[] = { &(msg->version), &(msg->services), &(msg->timestamp), &(msg->addr_recv_services),
				&(msg->addr_recv_IP_address), &(msg->addr_recv_port), &(msg->addr_trans_services), &(msg->addr_trans_IP_address),
				&(msg->addr_trans_port), &(msg->nonce), &(msg->user_agent_bytes), &(msg->start_height)
		};
	    size_t len = 0;
		for(int i = 0; i < sizeof(n)/sizeof(n[0]); ++i){
			len += n[i];
			ret = sdscatlen(ret, from[i], n[i]);
		}
		compute_checksum((uint8_t *)ret + BTC_HDR_SIZE, len, header.checksum);
		header.length = len;

		if(pl) pl->version = version;
	}
	else if(strncmp(command, "ping", 4) == 0){
	}
	else if(strncmp(command, "verack", 6) == 0){
		compute_checksum((uint8_t *)ret + BTC_HDR_SIZE, 0, header.checksum);
		header.length = 0;
	}
	else{
	}

    memcpy(ret, &header, BTC_HDR_SIZE);
    if(hdr) *hdr = header;

    return ret;
}


