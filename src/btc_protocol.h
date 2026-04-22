/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BTC_PROTOCOL_H
#define BTC_PROTOCOL_H
#include <arpa/inet.h>
#include <hp/sdsinc.h>

// Bitcoin network message header
typedef struct {
    uint8_t magic[4];
    char command[12];
    uint32_t length;
    uint8_t checksum[4];
} MessageHeader;
#define BTC_HDR_SIZE (sizeof(MessageHeader))

sds btc_p2p_ver_new();

#endif
