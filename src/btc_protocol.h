/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BTC_PROTOCOL__H
#define BTC_PROTOCOL__H
#include <stdint.h>
#include "hp/sdsinc.h"
// Bitcoin network message header
typedef struct {
    uint8_t magic[4];
    char command[12];
    uint32_t length;
    uint8_t checksum[4];
} btc_p2p_hdr;

#define BTC_HDR_SIZE (sizeof(btc_p2p_hdr))

typedef struct {
	int32_t 	version;
	uint64_t 	services;
	int64_t 	timestamp;
	uint64_t 	addr_recv_services;
	char	    addr_recv_IP_address[16];
	uint16_t	addr_recv_port;
	uint64_t	addr_trans_services;
	char		addr_trans_IP_address[16];
	uint16_t	addr_trans_port	;
	uint64_t	nonce;
	uint8_t 	user_agent_bytes; 	//compactSize
	char		user_agent[32];		//Required if user_agent bytes > 0
	int32_t		start_height;
	bool		relay;
} CVersionMsg;

typedef union {
	CVersionMsg version;
} btc_p2p_payload;

sds btc_p2pmsg_new(const char *command, void * payload, btc_p2p_hdr * hdr, btc_p2p_payload * pl);

#endif //BTC_PROTOCOL__H
