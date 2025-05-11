// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BTC_PROTOCOL_H
#define BTC_PROTOCOL_H
#include <arpa/inet.h>

typedef struct {
    int32_t 	version;
    uint64_t 	services;
    int64_t 	timestamp;
    int64_t 	service;	//   - 8 bytes (service bits)
    char 		addrme[8];
    char		v[10];
} CVersionMsg;

// Bitcoin network message header
typedef struct {
    uint8_t magic[4];
    char command[12];
    uint32_t length;
    uint8_t checksum[4];
} MessageHeader;
#define BTC_HDR_SIZE (sizeof(MessageHeader))

void compute_checksum(const uint8_t *data, size_t len, uint8_t *checksum);
// Serialize CVersionMsg (simplified, no full Bitcoin serialization)
size_t serialize_version_msg(const CVersionMsg *msg, uint8_t *buffer, size_t max_len);
#endif
