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

typedef union {
	struct VERSION {
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
	} version;
	struct WTXIDRELAY{ char c[0]; } wtxidrelay;
	struct { char c[0]; } sendaddrv2;
	struct PONG { int64_t c; } pong;
	struct SENDCMPCT { bool sendcmpct_hb; uint64_t sendcmpct_version; } sendcmpct;
	struct CMPCTBLOCK { char header[80]; } cmpctblock;
	struct HEADERS { uint8_t count; char block_hdr[3][80]; } headers; //<-getheaders
	struct { char c[0]; } feefilter;
} btc_p2p_payload;

/**!
 * @param inhdr: NULL for "version" message
 */
sds btc_p2pmsg_reply(const btc_p2p_hdr *inhdr, btc_p2p_payload const * inpl, btc_p2p_hdr * outhdr, btc_p2p_payload * outpl);

/*
 * @return: 0 if OK
 * */
int btc_p2pmsg_parse(char const * buf, size_t  len, btc_p2p_hdr * hdr, btc_p2p_payload * pl);

#endif //BTC_PROTOCOL__H
