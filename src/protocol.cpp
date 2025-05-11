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
#include <arpa/inet.h>
#include <openssl/sha.h> // For SHA-256 checksum
#include "protocol.h"
// Compute SHA-256 checksum (first 4 bytes of double SHA-256)
void compute_checksum(const uint8_t *data, size_t len, uint8_t *checksum) {
    uint8_t hash1[SHA256_DIGEST_LENGTH];
    uint8_t hash2[SHA256_DIGEST_LENGTH];
    SHA256(data, len, hash1);
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);
    memcpy(checksum, hash2, 4);
}

//typedef struct {
//    int32_t 	version;
//    uint64_t 	services;
//    int64_t 	timestamp;
//    int64_t 	service;	//   - 8 bytes (service bits)
//    char 		addrme[8];
//    CNetAddr::Encoding	v;
//} CVersionMsg;

// Serialize CVersionMsg (simplified, no full Bitcoin serialization)
size_t serialize_version_msg(const CVersionMsg *msg, uint8_t *buffer, size_t max_len)
{
	if(!(msg && buffer && max_len > 0)) return -1;
	if(max_len < sizeof(CVersionMsg)) return -2;
    size_t pos = 0;
    int n[] = { sizeof(int32_t), sizeof(uint64_t), sizeof(int64_t),
    			sizeof(int64_t), sizeof(char[8]), sizeof(char[10])};
    void const * from[] = { &(msg->version), &(msg->services), &(msg->timestamp)
    		, &(msg->service), (msg->addrme), &(msg->v)};
    for(int i = 0; i < sizeof(n) / sizeof(n[0]); ++i){
        memcpy(buffer + pos, from[i], n[i]);
        pos += n[i];
    }
    return pos;
}
