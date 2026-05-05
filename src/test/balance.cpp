/*!
 * private data
 * tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5
 * 001 invoice:
 * wallet_565,lntbs11970n1p5k202qpp53n3ecycdktgjlyz0prj7gunxcvq0ha0rz40j54kr7acmw832jlusdqswaskcmr9w30n2d34cqzzsxq9yn4qqsp56487dpcg226vj8ux5c7u4c3nyuywdrs5enxgglt5n6eqfej957fq9qxpqysgq6292r3j4zwsldx2z7fz9y5zgg4rk5czfvejv9je9v5x0783gntu8e57lsph7czwtjg3czg0g5j3m0rc34p99jzwe33djysdum6xvugsqtq6u96,8ce39c130db2d12f904f08e5e47266c300fbf5e3155f2a56c3f771b71e2a97f9
 *
 * 003 invoice:
 * wallet_565,lntbs90940n1p5k205rpp58wc7epu92h3fca0quclwdrhqks3q5k4ej306tl04m0prc0xj6s5qdqswaskcmr9w30n2d34cqzzsxq9yn4qqsp52uyx8xrtr9t9e76cvmwullus0t7u83prfjn45hm3jfvplpcv770s9qxpqysgqhaegmug6h6220egrdm4msa4f2r2e7rnu7z2d5ldqja9tdsy4vl9h384lpvuswm9kn9vkh5jd49m8vs2vqrp7x9mwqnm0q409c4k3zrcp8azr6g,3bb1ec878555e29c75e0e63ee68ee0b4220a5ab9945fa5fdf5dbc23c3cd2d428
 *
 */

#include "bitcoin-build-config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <cassert>
#include <string>

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

#ifdef SIGNET_BALANCE
#include "hp/sdsinc.h"
// #include "hp/hp_cjson.h"
#include "hp/hp_str.h"
#else
#include <key.h>
#include <key_io.h>
#include <crypto/hmac_sha512.h>
#endif

/* ---------- 常量与配置 ---------- */
static const char *BITCOIN_CLI_BASE = "bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet-wallet-1-balance-hongjunliao/data/0";
static const size_t MAX_ADDR_PER_BATCH = 2;   /* 分批查询 listunspent，避免命令行过长 */
static const uint32_t HARDENED_OFFSET = 0x80000000u;
static const size_t MAX_RANGE = 2000;          /* 用户要求的上限 */

/* ---------- 简单的 Base58Check 解码（用于 xprv 解码） ---------- */
static const char *BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static
int base58_decode(const char *in, unsigned char *out, size_t *outlen)
{
    /* 简单实现：将 base58 转为大整数，再输出二进制，大端 */
    size_t in_len = strlen(in);
    size_t i, j;
    /* 大整数以 big-endian 存储在数组 bignum 中，每个元素为 base 256 的字节 */
    size_t bignum_len = (in_len * 733) / 1000 + 1; /* log(58)/log(256) ≈ 0.733 */
    auto bignum = (unsigned char *)calloc(bignum_len, 1);
    if (!bignum) return -1;

    for (i = 0; i < in_len; ++i)
    {
        const char *p = strchr(BASE58_ALPHABET, in[i]);
        if (!p)
        {
            free(bignum);
            return -2;
        }
        int carry = (int)(p - BASE58_ALPHABET);
        for (j = 0; j < bignum_len; ++j)
        {
            int v = bignum[j] * 58 + carry;
            bignum[j] = v & 0xFF;
            carry = v >> 8;
        }
        if (carry != 0)
        {
            free(bignum);
            return -3; /* overflow (shouldn't happen with our size estimate) */
        }
    }

    /* count leading zeros in input => leading 0x00 bytes in output */
    size_t zeros = 0;
    for (i = 0; i < in_len && in[i] == '1'; ++i) ++zeros;

    /* trim leading zero bytes from big-endian conversion (we used little-endian) */
    /* Our bignum is little-endian: lowest byte at index 0. We need to reverse for big-endian output. */
    /* Compute actual length of bignum without leading zero bytes (from high end) */
    ssize_t idx = bignum_len - 1;
    while (idx >= 0 && bignum[idx] == 0) --idx;
    size_t bin_len = zeros + (idx + 1);

    if (*outlen < bin_len)
    {
        free(bignum);
        *outlen = bin_len;
        return -4;
    }

    /* output: leading zeros then big-endian bytes */
    memset(out, 0, bin_len);
    for (i = 0; i < zeros; ++i) out[i] = 0x00;
    for (j = 0; j <= (size_t)idx; ++j)
    {
        out[zeros + j] = bignum[idx - j];
    }

    *outlen = bin_len;
    free(bignum);
    return 0;
}

/* double SHA256 (用于 base58check 校验) */
static
void sha256d(const unsigned char *data, size_t len, unsigned char *out32)
{
    #ifdef SIGNET_BALANCE
        unsigned char tmp[SHA256_DIGEST_LENGTH]; 
        SHA256(data, len, tmp);
        SHA256(tmp, SHA256_DIGEST_LENGTH, out32);
    #else
        unsigned char tmp[SHA256_DIGEST_LENGTH]; 
        CSHA256().Write((const unsigned char*)data, len).Finalize(tmp);
        CSHA256().Write((const unsigned char*)tmp, SHA256_DIGEST_LENGTH).Finalize(out32);
    #endif
}

/* ---------- BIP32 派生（使用 HMAC-SHA512） ---------- */
/* 输入:
     parent_priv32: 32 bytes 私钥
     parent_chaincode: 32 bytes
     index: uint32_t
   输出:
     child_priv32: 32 bytes
     child_chaincode: 32 bytes
   返回: 0 成功, 非0 失败
*/
static
int bip32_ckd_priv(const unsigned char parent_priv32[32],
                    const unsigned char parent_chaincode[32],
                    uint32_t index,
                    unsigned char child_priv32[32],
                    unsigned char child_chaincode[32],
                    secp256k1_context *ctx)
{
    unsigned char data[1 + 33 + 4];
    unsigned char I[64];

    if (index & HARDENED_OFFSET)
    {
        /* hardened: data = 0x00 || ser256(kpar) || ser32(index) */
        data[0] = 0x00;
        memcpy(data + 1, parent_priv32, 32);
        /* prepend 0x00 to make 33 bytes of key data for HMAC */
        /* 对私钥执行 HMAC */
    }
    else
    {
        /* non-hardened: data = serP(point(kpar)) || ser32(index) */
        /* 使用 libsecp256k1 得到压缩公钥 */
        secp256k1_pubkey pub;
        size_t outlen = 33;
        unsigned char out[33];
        if (!secp256k1_ec_pubkey_create(ctx, &pub, parent_priv32))
        {
            return -1;
        }
        secp256k1_ec_pubkey_serialize(ctx, out, &outlen, &pub, SECP256K1_EC_COMPRESSED);
        memcpy(data, out, 33);
    }

    /* append index big-endian */
    data[(index & HARDENED_OFFSET) ? (1 + 32) : 33] = (index >> 24) & 0xFF;
    data[(index & HARDENED_OFFSET) ? (1 + 32) : 33 + 1] = (index >> 16) & 0xFF;
    data[(index & HARDENED_OFFSET) ? (1 + 32) : 33 + 2] = (index >> 8) & 0xFF;
    data[(index & HARDENED_OFFSET) ? (1 + 32) : 33 + 3] = (index) & 0xFF;

    /* HMAC-SHA512(key = parent_chaincode, data) */
    unsigned int lenI = 0;
    #ifdef SIGNET_BALANCE
        HMAC(EVP_sha512(), parent_chaincode, 32, data, (index & HARDENED_OFFSET) ? (1 + 32 + 4) : (33 + 4), I, &lenI);
    #else
        CHMAC_SHA512(parent_chaincode, 32).Write(data, (index & HARDENED_OFFSET) ? (1 + 32 + 4) : (33 + 4)).Finalize(I);
        lenI = 64;
    #endif
    if (lenI != 64) return -2;

    /* IL, IR */
    unsigned char IL[32], IR[32];
    memcpy(IL, I, 32);
    memcpy(IR, I + 32, 32);
    /* child_priv = (IL + kpar) mod n */
    /* 使用 libsecp256k1 的 scalar 加法：通过把 IL 解释为 32-byte scalar 与 private key相加 */
    /* libsecp256k1 没有直接的标量加法 API暴露，但可以通过 secp256k1_ec_privkey_tweak_add 做私钥 tweak */
    memcpy(child_priv32, parent_priv32, 32);
    if (!secp256k1_ec_seckey_tweak_add(ctx, child_priv32, IL))
    {
        return -3; /* tweak 加法失败，通常是 IL 为零或导致无效私钥 -> 可按 BIP32 规范该索引无效，应尝试下一个索引 */
    }
    memcpy(child_chaincode, IR, 32);
    return 0;
}

/* ---------- Taproot tweak（BIP341） ---------- */
/* Tagged hash: SHA256(SHA256(tag) || SHA256(tag) || msg) */
static
void sha256_tagged(const char *tag, const unsigned char *msg, size_t msglen, unsigned char out32[32])
{
    #ifdef SIGNET_BALANCE

    unsigned char tag_hash[32];
    SHA256((const unsigned char *)tag, strlen(tag), tag_hash);

    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, tag_hash, 32);
    SHA256_Update(&ctx, tag_hash, 32);
    if (msg && msglen)
    {
        SHA256_Update(&ctx, msg, msglen);
    }
    SHA256_Final(out32, &ctx);
    #endif
}

/* 给定 internal x-only pubkey（32 bytes），计算 tweak（按 BIP341, empty merkle root）*/
static
void taproot_tweak_calc(const unsigned char xonly[32], unsigned char out_tweak32[32])
{
    /* tagged hash "TapTweak" */
    sha256_tagged("TapTweak", xonly, 32, out_tweak32);
}

/* 将私钥对应的 xonly pubkey tweak 得到 output pubkey（xonly） */
/* 输入: priv32 (32), ctx 已初始化
   输出: out_xonly (32), 若返回非0表示成功
*/
static
int derive_taproot_output_xonly_from_priv(const unsigned char priv32[32],
                                          unsigned char out_xonly[32],
                                          secp256k1_context *ctx)
{
    secp256k1_keypair keypair;
    /* 创建 keypair（私钥 -> keypair） */
    if (!secp256k1_keypair_create(ctx, &keypair, priv32))
    {
        return -1;
    }

    /* 提取 internal x-only pubkey */
    secp256k1_xonly_pubkey internal_xonly;
    int pk_parity = 0;
    if (!secp256k1_keypair_xonly_pub(ctx, &internal_xonly, &pk_parity, &keypair))
    {
        return -2;
    }

    unsigned char internal_xonly_bytes[32];
    if (!secp256k1_xonly_pubkey_serialize(ctx, internal_xonly_bytes, &internal_xonly))
    {
        return -3;
    }

    /* 计算 tweak = tagged_hash("TapTweak", internal_xonly_bytes) 
       这里调用 secp256k1_tagged_sha256：不同版本签名可能不同（有的需要 ctx，有的直接是 (out, tag, taglen, msg, msglen)）
       如果你本地的签名不同，请按实际签名调整调用。 */
    unsigned char tweak32[32];
    if (!secp256k1_tagged_sha256(ctx, tweak32, (const unsigned char *)"TapTweak", strlen("TapTweak"), internal_xonly_bytes, 32))
    {
        return -4;
    }

    /* 使用库函数将 keypair 做 xonly_tweak_add（同样注意实际签名，名称可能为 secp256k1_keypair_xonly_tweak_add） */
    if (!secp256k1_keypair_xonly_tweak_add(ctx, &keypair, tweak32))
    {
        return -5;
    }

    /* 从 tweak 后的 keypair 提取输出 x-only pubkey 并序列化为 32 字节 */
    secp256k1_xonly_pubkey output_xonly;
    if (!secp256k1_keypair_xonly_pub(ctx, &output_xonly, NULL, &keypair))
    {
        return -6;
    }
    if (!secp256k1_xonly_pubkey_serialize(ctx, out_xonly, &output_xonly))
    {
        return -7;
    }

    return 0;
}
/* ---------- bech32m 编码（用于 Taproot 地址，witness v1 + 32 字节 program） ---------- */
/* 本处实现为 Bech32m minimal functional implementation，参考 BIP-173/350。 */

/* bech32 charset */
static const char BECH32_CHARSET[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

static
int bech32_polymod_step(uint32_t pre)
{
    uint32_t b = pre >> 25;
    return ((pre & 0x1FFFFFF) << 5) ^
           (-((b >> 0) & 1) & 0x3b6a57b2) ^
           (-((b >> 1) & 1) & 0x26508e6d) ^
           (-((b >> 2) & 1) & 0x1ea119fa) ^
           (-((b >> 3) & 1) & 0x3d4233dd) ^
           (-((b >> 4) & 1) & 0x2a1462b3);
}

/* convertbits: general base conversion */
static
int convert_bits(unsigned char *out, size_t *outlen, int outbits,
                 const unsigned char *in, size_t inlen, int inbits, int pad)
{
    unsigned int acc = 0;
    int bits = 0;
    size_t max_out = *outlen;
    size_t index = 0;
    unsigned int maxv = (1u << outbits) - 1u;
    size_t i;
    for (i = 0; i < inlen; ++i)
    {
        acc = (acc << inbits) | in[i];
        bits += inbits;
        while (bits >= outbits)
        {
            bits -= outbits;
            if (index >= max_out) return -1;
            out[index++] = (acc >> bits) & maxv;
        }
    }
    if (pad)
    {
        if (bits)
        {
            if (index >= max_out) return -1;
            out[index++] = (acc << (outbits - bits)) & maxv;
        }
    }
    else
    {
        if (bits >= inbits) return -2;
        if (((acc << (outbits - bits)) & maxv) != 0) return -3;
    }
    *outlen = index;
    return 0;
}

/* bech32m checksum */
static
int bech32_create_checksum(const char *hrp, const unsigned char *data, size_t datalen, unsigned char *checksum_out /*6 bytes*/)
{
    /* expand HRP */
    size_t hrp_len = strlen(hrp);
    size_t expand_len = hrp_len * 2 + 1;
    auto expand = (unsigned char *)malloc(expand_len);
    if (!expand) return -1;
    size_t i;
    for (i = 0; i < hrp_len; ++i) expand[i] = (hrp[i] >> 5);
    expand[hrp_len] = 0;
    for (i = 0; i < hrp_len; ++i) expand[hrp_len + 1 + i] = (hrp[i] & 31);

    uint32_t chk = 1;
    /* polymod over expand */
    for (i = 0; i < expand_len; ++i)
    {
        int top = chk >> 25;
        chk = ((chk & 0x1FFFFFF) << 5) ^ expand[i];
        if (top & 1) chk ^= 0x3b6a57b2;
        if (top & 2) chk ^= 0x26508e6d;
        if (top & 4) chk ^= 0x1ea119fa;
        if (top & 8) chk ^= 0x3d4233dd;
        if (top & 16) chk ^= 0x2a1462b3;
    }
    /* polymod over data */
    for (i = 0; i < datalen; ++i)
    {
        int top = chk >> 25;
        chk = ((chk & 0x1FFFFFF) << 5) ^ data[i];
        if (top & 1) chk ^= 0x3b6a57b2;
        if (top & 2) chk ^= 0x26508e6d;
        if (top & 4) chk ^= 0x1ea119fa;
        if (top & 8) chk ^= 0x3d4233dd;
        if (top & 16) chk ^= 0x2a1462b3;
    }

    /* bech32m requires constant 0x2bc830a3 */
    uint32_t const_bech32m = 0x2bc830a3u;
    uint32_t v = chk ^ const_bech32m;

    /* produce 6 values */
    for (i = 0; i < 6; ++i)
    {
        checksum_out[i] = (v >> (5 * (5 - i))) & 0x1F;
    }

    free(expand);
    return 0;
}

/* encode bech32m (hrp + witness version + program) */
/* witness_version: 0..16, program bytes pointer with prog_len */
/* outbuf must be big enough (e.g., 200) */
static
int bech32m_encode(char *outbuf, size_t outbuflen, const char *hrp, int witness_version, const unsigned char *program, size_t prog_len)
{
    unsigned char data[1 + 65]; /* 1 for version + converted program up to 65 */
    size_t data_len = 1;
    data[0] = witness_version; /* version as integer */

    /* convert program bytes (8-bit) to 5-bit array */
    unsigned char prog5[100];
    size_t prog5_len = sizeof(prog5);
    if (convert_bits(prog5, &prog5_len, 5, program, prog_len, 8, 1) != 0) return -1;

    if (1 + prog5_len + 6 + strlen(hrp) + 1 > outbuflen) return -2;

    memcpy(data + 1, prog5, prog5_len);
    data_len = 1 + prog5_len;

    unsigned char checksum[6];
    if (bech32_create_checksum(hrp, data, data_len, checksum) != 0) return -3;

    /* assemble string hrp + '1' + data + checksum (map chars) */
    size_t pos = 0;
    size_t hrplen = strlen(hrp);
    memcpy(outbuf + pos, hrp, hrplen);
    pos += hrplen;
    outbuf[pos++] = '1';
    size_t i;
    for (i = 0; i < data_len; ++i)
    {
        if (data[i] >= 32) return -4;
        outbuf[pos++] = BECH32_CHARSET[data[i]];
    }
    for (i = 0; i < 6; ++i)
    {
        outbuf[pos++] = BECH32_CHARSET[checksum[i]];
    }
    outbuf[pos] = 0;
    return 0;
}

/* ---------- descriptor 解析（针对给定格式 tr(tprv.../path/*)#...） ---------- */
/* 仅实现用户提供示例所需的解析能力：提取 xprv base58，解析路径 tokens（支持数字 + optional 'h'/'H' 表示 hardened），并识别尾部的 '*' */
static
int parse_descriptor_tr_tprv(const char *desc,
                             char *out_xprv_base58, size_t out_xprv_len,
                             uint32_t *path_tokens, size_t *path_len, /* tokens before wildcard */
                             int *has_wildcard)
{
    /* 简单查找 "tr(" ... ")" */
    const char *p = strstr(desc, "tr(");
    if (!p) return -1;
    p += 3;
    const char *q = strchr(p, ')');
    if (!q) return -2;
    size_t inner_len = q - p;
    auto inner = (char *)malloc(inner_len + 1);
    if (!inner) return -3;
    memcpy(inner, p, inner_len);
    inner[inner_len] = 0;

    /* inner 应该以 tprv... 开头并包含路径 /.../* */
    /* 找到第一个 '/'，base58 在前 */
    char *slash = strchr(inner, '/');
    if (!slash)
    {
        /* 可能没有路径（不太可能），直接取 inner 作为 xprv */
        if (strlen(inner) >= out_xprv_len) { free(inner); return -4; }
        strcpy(out_xprv_base58, inner);
        *path_len = 0;
        *has_wildcard = 0;
        free(inner);
        return 0;
    }
    size_t xprv_len = slash - inner;
    if (xprv_len >= out_xprv_len) { free(inner); return -5; }
    memcpy(out_xprv_base58, inner, xprv_len);
    out_xprv_base58[xprv_len] = 0;

    /* 解析后续路径 token，直到遇到 '*' 或结束 */
    char *rest = slash + 1;
    size_t idx = 0;
    *has_wildcard = 0;
    char *tok = strtok(rest, "/");
    while (tok != NULL && idx < *path_len + 1000)
    {
        if (strcmp(tok, "*") == 0)
        {
            *has_wildcard = 1;
            break;
        }
        /* token 形如 86h 或 1 或 0h ... */
        size_t tlen = strlen(tok);
        int hardened = 0;
        if (tlen > 0 && (tok[tlen - 1] == 'h' || tok[tlen - 1] == 'H' || tok[tlen - 1] == '\''))
        {
            hardened = 1;
            tok[tlen - 1] = 0;
        }
        long val = strtol(tok, NULL, 10);
        if (val < 0 || val > 0x7fffffff) { free(inner); return -6; }
        uint32_t v = (uint32_t)val;
        if (hardened) v |= HARDENED_OFFSET;
        path_tokens[idx++] = v;
        tok = strtok(NULL, "/");
    }
    *path_len = idx;
    free(inner);
    return 0;
}

/* ---------- 从 xprv(base58) 读取私钥与 chaincode（解析 extended private key） ---------- */
/* xprv base58check 格式: 4 bytes version || 1 depth || 4 parent fingerprint || 4 childnum || 32 chaincode || 33 keydata || 4 checksum
   对 tprv 的 version bytes 为 0x04358394 (testnet xprv) 或各种网的 prefix；我们这里只解析并验证 checksum，然后取 chaincode 与 keydata
*/
static
int decode_xprv_base58(const char *xprv_base58, unsigned char out_priv32[32], unsigned char out_chaincode[32])
{
    unsigned char buf[200];
    size_t buflen = sizeof(buf);
    int rc = base58_decode(xprv_base58, buf, &buflen);
    if (rc != 0) return -1;
    if (buflen < 4 + 1 + 4 + 4 + 32 + 33 + 4) return -2; /* not likely */

    /* 最后 4 个字节是 checksum */
    unsigned char chk[32];
    sha256d(buf, buflen - 4, chk);
    if (memcmp(chk, buf + buflen - 4, 4) != 0)
    {
        return -3; /* checksum mismatch */
    }

    size_t pos = 0;
    /* version 4 */
    pos += 4;
    /* depth 1 */
    pos += 1;
    /* parent fingerprint 4 */
    pos += 4;
    /* child number 4 */
    pos += 4;
    /* chaincode 32 */
    if (pos + 32 > buflen) return -4;
    memcpy(out_chaincode, buf + pos, 32);
    pos += 32;
    /* keydata 33: first byte 0x00 then 32 bytes privkey for xprv */
    if (pos + 33 > buflen) return -5;
    if (buf[pos] != 0x00) return -6;
    memcpy(out_priv32, buf + pos + 1, 32);
    return 0;
}

/* ---------- Bitcoin RPC 调用（通过 bitcoin-cli 命令行） ---------- */
/* 使用 listunspent 查询给定地址列表的 UTXO，返回 JSON 输出作为字符串（caller 负责 free） */
static
char *bitcoin_cli_listunspent_for_addresses(char **addrs, size_t addr_count)
{
    /* 构建 JSON 数组字符串 e.g. '["addr1","addr2",...]' */
    size_t i;
    size_t est = 2 + addr_count * 40;
    auto json = (char *)malloc(est);
    if (!json) return NULL;
    strcpy(json, "[");
    for (i = 0; i < addr_count; ++i)
    {
        /* escape not implemented: assuming addresses 不包含特殊字符 */
        strcat(json, "\"");
        strcat(json, addrs[i]);
        strcat(json, "\"");
        if (i + 1 < addr_count) strcat(json, ",");
    }
    strcat(json, "]");

    /* 构建命令: bitcoin-cli -signet -datadir=~/data/0 listunspent 0 9999999 'JSON' */
    size_t cmdlen = strlen(BITCOIN_CLI_BASE) + 100 + strlen(json) + 10;
    auto cmd = (char *)malloc(cmdlen);
    if (!cmd) { free(json); return NULL; }
    snprintf(cmd, cmdlen, "%s listunspent 0 9999999 '%s'", BITCOIN_CLI_BASE, json);
    free(json);

    /* popen 读取输出 */
    FILE *fp = popen(cmd, "r");
    free(cmd);
    if (!fp) return NULL;
    char *out = NULL;
    size_t outcap = 0;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
    {
        auto tmp = (char *)realloc(out, outcap + n + 1);
        if (!tmp) { free(out); pclose(fp); return NULL; }
        out = tmp;
        memcpy(out + outcap, buf, n);
        outcap += n;
        out[outcap] = 0;
    }
    pclose(fp);
    return out;
}

/* 解析 listunspent 的 JSON 输出，累加 "amount" 字段的数值（BTC），返回总额（double） */
/* 这里做最小化的 JSON 解析：在字符串中查找 "amount": 并用 strtod 读取其后浮点数 */
static
double sum_amounts_from_listunspent_json(const char *json)
{
    double total = 0.0;
    const char *p = json;
    while ((p = strstr(p, "\"amount\"")) != NULL)
    {
        /* find ':' */
        const char *c = strchr(p, ':');
        if (!c) break;
        ++c;
        /* skip spaces */
        while (*c && isspace((unsigned char)*c)) ++c;
        char *endptr = NULL;
        double v = strtod(c, &endptr);
        if (endptr && endptr != c)
        {
            total += v;
            p = endptr;
        }
        else
        {
            p = c + 1;
        }
    }
    return total;
}


/* ---------- RPC: 使用 scantxoutset start 查询给定地址列表的 total_amount ---------- */
/* 返回: >=0 表示 BTC 金额总和；负值表示错误（无法执行 RPC、解析失败等） */
static
double bitcoin_cli_scantxoutset_for_addresses(char **addrs, size_t addr_count)
{
    if (addr_count == 0) return 0.0;

    /* 构建 JSON 数组，元素为 "addr(<address>)" */
    auto json = std::string("[");
    for (size_t i = 0; i < addr_count; ++i)
    {
        json = json + "\"addr(" + std::string(addrs[i]) + ")\",";
    }
    json[json.length() - 1] = ']'; /* 替换最后一个逗号为 ] */;

    /* scantxoutset start '<array>' */
    auto cmd = std::string(BITCOIN_CLI_BASE) + " scantxoutset start '" + json + "'";

    FILE *fp = popen(cmd.c_str(), "r");
    printf("%s: %s\n", __FUNCTION__, cmd.c_str());
    if (!fp) return -2.0;

    std::string out;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
    {
        out = out + std::string(buf, n);
    }
    int status = pclose(fp);
    (void)status;

    printf("%s: '%s'\n", __FUNCTION__, out.c_str());

    /* 在 JSON 中查找 "total_amount" 字段并解析其值 */
    double total = -5.0;
    const char *p = strstr(out.c_str(), "\"total_amount\"");
    if (p != NULL)
    {
        const char *c = strchr(p, ':');
        if (c)
        {
            ++c;
            while (*c && isspace((unsigned char)*c)) ++c;
            char *endptr = NULL;
            double v = strtod(c, &endptr);
            if (endptr && endptr != c)
            {
                total = v;
            }
        }
    }

    return total;
}

/* ---------- 顶层逻辑: 对 descriptor 派生地址并查询余额 ---------- */
/* balance_main: 程序主入口（替代 main） */
int balance_main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /* 示例 descriptor — 在实际使用可以替换为其它输入来源 */
    const char *descriptor = "tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5";

    /* parse descriptor */
    char xprv[200];
    uint32_t path_tokens[16];
    size_t path_len = 16;
    int has_wildcard = 0;
    memset(xprv, 0, sizeof(xprv));
    if (parse_descriptor_tr_tprv(descriptor, xprv, sizeof(xprv), path_tokens, &path_len, &has_wildcard) != 0)
    {
        fprintf(stderr, "Failed to parse descriptor\n");
        return 1;
    }
    if (!has_wildcard)
    {
        fprintf(stderr, "Descriptor has no wildcard; nothing to derive\n");
        return 1;
    }

    /* decode xprv base58 -> priv32 & chaincode */
    unsigned char master_priv[32];
    unsigned char master_chaincode[32];
    if (decode_xprv_base58(xprv, master_priv, master_chaincode) != 0)
    {
        fprintf(stderr, "Failed to decode xprv base58\n");
        return 2;
    }

    #ifndef SIGNET_BALANCE
        CExtKey extkey, extkey2;
        extkey = DecodeExtKey(xprv); extkey2.Decode(master_priv);
        assert(extkey.key.IsValid());
        assert(extkey.chaincode == uint256(master_chaincode));
        assert(memcmp(extkey.key.data(), master_priv, 32) == 0);
    #endif
    /* init secp context */
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    if (!ctx)
    {
        fprintf(stderr, "Failed to create secp256k1 context\n");
        return 3;
    }

    /* derive the parent key at path_tokens (all tokens before the wildcard) */
    unsigned char cur_priv[32];
    unsigned char cur_chaincode[32];
    memcpy(cur_priv, master_priv, 32);
    memcpy(cur_chaincode, master_chaincode, 32);

    for (size_t i = 0; i < path_len; ++i)
    {
        uint32_t idx = path_tokens[i];
        int rc = bip32_ckd_priv(cur_priv, cur_chaincode, idx, cur_priv, cur_chaincode, ctx);
        if (rc != 0)
        {
            fprintf(stderr, "BIP32 derivation failed at path idx %zu (token %" PRIu32 ")\n", i, idx);
            secp256k1_context_destroy(ctx);
            return 4;
        }
        #ifndef SIGNET_BALANCE
            assert(extkey.Derive(extkey, idx)); 
        #endif
    }
    #ifndef SIGNET_BALANCE
        assert(memcmp(extkey.key.data(), cur_priv, 32) == 0);
        assert(extkey.chaincode == uint256(cur_chaincode));
    #endif

    /* range: 0..N-1 ; 要求 range <= 2000 */
    size_t range = 2000;
    if (range > MAX_RANGE) range = MAX_RANGE;

    /* 预分配地址字符串数组（malloc'd C 字符串） */
    auto addresses = (char **)calloc(range, sizeof(char *));
    assert(addresses);

    /* derive each child private (non-hardened) with index i, then compute taproot address (bech32m, tb1...) */
    for (size_t i = 0; i < range; ++i)
    {
        /* copy parent key and chaincode, then derive child at index i (non-hardened) */
        unsigned char child_priv[32];
        unsigned char child_chaincode[32];
        if (bip32_ckd_priv(cur_priv, cur_chaincode, (uint32_t)i, child_priv, child_chaincode, ctx) != 0)
        {
            /* derivation error: skip this index */
            addresses[i] = NULL;
            continue;
        }

        #ifndef SIGNET_BALANCE
            assert(extkey.Derive(extkey2, i)); 
            assert(extkey2.key.IsValid());
            assert(memcmp(extkey2.key.data(), child_priv, 32) == 0);
        #endif
        /* derive taproot output xonly pubkey */
        unsigned char out_xonly[32];
        if (derive_taproot_output_xonly_from_priv(child_priv, out_xonly, ctx) != 0)
        {
            addresses[i] = NULL;
            continue;
        }

        /* bech32m encode: hrp for signet/testnet 使用 "tb"（signet 与 testnet 同样使用 tb 前缀）
           witness version = 1, program = 32-byte xonly */
        char addrbuf[200]="";
        if (bech32m_encode(addrbuf, sizeof(addrbuf), "tb", 1, out_xonly, 32) != 0)
        {
            addresses[i] = NULL;
            continue;
        }

        addresses[i] = strdup(addrbuf);

    }

    /* Query balances in batches using listunspent for addresses (only RPC usage in this program) */
    double total_btc = 0.0;
    /* 按批次调用 scantxoutset */
    char *batch_addrs[MAX_ADDR_PER_BATCH];
    size_t batch_count = 0;
    for (size_t i = 0; i < range; ++i)
    {
        if (addresses[i])
        {
            batch_addrs[batch_count++] = addresses[i];
            if (batch_count >= MAX_ADDR_PER_BATCH)
            {
                double batch_total = bitcoin_cli_scantxoutset_for_addresses(batch_addrs, batch_count);
                if (batch_total >= 0.0)
                {
                    total_btc += batch_total;
                }
                /* reset batch */
                batch_count = 0;
            }
        }
    }
    if (batch_count > 0)
    {
        double batch_total = bitcoin_cli_scantxoutset_for_addresses(batch_addrs, batch_count);
        if (batch_total >= 0.0)
        {
            total_btc += batch_total;
        }
    }

    printf("wallet_565 %.8f BTC\n", total_btc);

    /* cleanup */
    for (size_t i = 0; i < range; ++i) if (addresses[i]) free(addresses[i]);
    free(addresses);
    secp256k1_context_destroy(ctx);
    return 0;
}
