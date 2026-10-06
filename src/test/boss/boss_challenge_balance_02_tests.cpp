#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>

// ======================================================================
// 初学者视角：实现步骤与关键概念注释
// ======================================================================

// 目标：
//   给定一个合法的 signet descriptor (tr(tprv.../*)#checksum)
//   1. 自己解析 xprv → master key
//   2. 按照 BIP32 路径 m/86'/1'/0'/0/* 推导出 0~1999 的私钥
//   3. 对于每个私钥 → 计算 Taproot 外部公钥（internal + tweak）
//   4. 生成对应的 tb1p... Bech32m 地址 (P2TR)
//   5. 把这些地址一次性喂给 bitcoin-cli scantxoutset start
//   6. 读取返回的 total_amount 作为余额

// 重要概念与参考链接（初学者必看）
// 1. BIP32 Hierarchical Deterministic Wallets          → https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki
// 2. BIP86 Taproot key derivation scheme               → https://github.com/bitcoin/bips/blob/master/bip-0086.mediawiki
// 3. BIP341 Taproot: SegWit version 1 spending rules   → https://github.com/bitcoin/bips/blob/master/bip-0341.mediawiki
// 4. BIP350 Bech32m new checksum for segwit v1+        → https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki
// 5. Descriptor tr() syntax                            → https://github.com/bitcoin/bitcoin/blob/master/doc/descriptors.md
// 6. scantxoutset RPC                                  → https://bitcoincore.org/en/doc/27.0.0/rpc/blockchain/scantxoutset/
// 7. signet                                        → https://github.com/bitcoin/bips/blob/master/bip-0325.mediawiki

// 实现步骤大纲（初学者视角）
// 步骤1. Base58Check 解码 tprv → 得到 version + depth + fingerprint + childnum + chaincode + privkey
// 步骤2. 校验 checksum (双 sha256 前4字节)
// 步骤3. 沿着路径 m/86'/1'/0'/0 推导 account 层扩展私钥 (BIP32 CKD)
// 步骤4. 对于 change=0 的每个 index (0~1999)，再做一次 CKD 得到 child 私钥
// 步骤5. child私钥 → secp256k1_keypair → xonly_pubkey (internal pubkey)
// 步骤6. 按照 BIP341 TapTweak：hash = TaggedHash("TapTweak", internal_pub || 0x00*32) 但这里无脚本树所以直接 tweak
// 步骤7. output_pub = internal + tweak (xonly tweak add)
// 步骤8. output xonly pubkey → 33字节 scriptPubKey = 0x01 + 32字节 xonlypub
// 步骤9. Bech32m 编码 hrp="tb" + witness version 1 + 32字节 pubkey → tb1p... 地址
// 步骤10. 把 2000 个地址组成 scantxoutset 的 descriptor 格式数组
// 步骤11. system() 或 popen() 调用 bitcoin-cli -signet scantxoutset start [...]
// 步骤12. 解析 JSON 中的 "total_amount"

// ======================================================================

static const char *b58alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static const char *bech32_charset = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

static const uint32_t bech32_generator[5] = {
    0x3b6a57b2UL, 0x26508e6dUL, 0x1ea119faUL, 0x3d4233ddUL, 0x2a1462b3UL
};

// bech32 相关的几个函数（BIP173 + BIP350 Bech32m）
// 初学者提示：Bech32 和 Bech32m 的唯一区别是最后的 constant 值不同
// Bech32   constant = 1
// Bech32m  constant = 0x2bc830a3
static uint32_t bech32_polymod(const uint8_t *values, size_t len) {
    uint32_t chk = 1;
    for (size_t p = 0; p < len; ++p) {
        uint8_t top = chk >> 25;
        chk = (chk & 0x1ffffff) << 5 ^ values[p];
        for (int i = 0; i < 5; ++i)
            if ((top >> i) & 1)
                chk ^= bech32_generator[i];
    }
    return chk;
}

static void bech32_hrp_expand(const char *hrp, uint8_t *out) {
    while (*hrp) *(out++) = *(hrp++) & 0x1f;
    *(out++) = 0;
}

static int bech32_convert_bits(uint8_t *out, size_t *outlen, int outbits,
                               const uint8_t *in, size_t inlen, int inbits, int pad) {
    uint32_t val = 0;
    int bits = 0;
    size_t written = 0;
    uint32_t maxv = (1 << outbits) - 1;
    for (size_t i = 0; i < inlen; ++i) {
        val = (val << inbits) | in[i];
        bits += inbits;
        while (bits >= outbits) {
            bits -= outbits;
            out[written++] = (val >> bits) & maxv;
        }
    }
    if (pad && bits) out[written++] = (val << (outbits - bits)) & maxv;
    else if (bits >= inbits || ((val << (outbits - bits)) & maxv)) return 0;
    *outlen = written;
    return 1;
}

static int bech32_encode(char *output, const char *hrp, const uint8_t *data, size_t data_len, int bech32m) {
    uint8_t data5[512];
    size_t data5_len = 0;
    if (!bech32_convert_bits(data5, &data5_len, 5, data, data_len, 8, 1)) return 0;

    uint8_t hrpexp[128];
    bech32_hrp_expand(hrp, hrpexp);
    size_t hrplen = strlen(hrp);

    uint8_t values[hrplen + 1 + data5_len + 6];
    memcpy(values, hrpexp, hrplen + 1);
    memcpy(values + hrplen + 1, data5, data5_len);
    memset(values + hrplen + 1 + data5_len, 0, 6);

    uint32_t mod = bech32_polymod(values, hrplen + 1 + data5_len + 6) ^ (bech32m ? 0x2bc830a3UL : 1UL);
    for (int i = 0; i < 6; ++i)
        values[hrplen + 1 + data5_len + i] = (mod >> 5 * (5 - i)) & 31;

    sprintf(output, "%s1", hrp);
    for (size_t i = 0; i < data5_len + 6; ++i)
        strcat(output, (char[]){bech32_charset[values[hrplen + 1 + i]], 0});
    return 1;
}

// Base58Check 解码（BIP32 扩展密钥使用 Base58Check 编码）
static int base58_decode(const char *str, unsigned char *out, size_t *outlen) {
    // ... （保持原实现，省略具体代码以节省空间，但保留注释）
    // 初学者注意：Base58Check = Base58 + 双SHA256前4字节校验
    // tprv 的 version bytes 是 0x04358394 (signet/testnet privkey)
    // ... 原代码保持不变 ...
}

// 校验 Base58Check 的 checksum
static int verify_checksum(const unsigned char *data, size_t len) {
    if (len != 82) return 0;
    unsigned char h1[32], h2[32];
    SHA256(data, 78, h1);
    SHA256(h1, 32, h2);
    return memcmp(h2, data + 78, 4) == 0;
}

// BIP32 CKD (Child Key Derivation) - 最重要的部分之一
// 初学者重点理解：hardened vs normal，数据格式不同
static int derive_child(const unsigned char *parent, unsigned char *child, uint32_t index, secp256k1_context *ctx) {
    // parent 格式： [4]version [1]depth [4]fingerprint [4]childnum [32]chaincode [1]0x00 [32]privkey
    int hardened = (index & 0x80000000) != 0;
    unsigned char data[37];
    size_t datalen;

    if (hardened) {
        data[0] = 0;
        memcpy(data + 1, parent + 46, 32);   // 私钥
        datalen = 33;
    } else {
        secp256k1_pubkey pub;
        size_t publen = 33;
        secp256k1_ec_pubkey_create(ctx, &pub, parent + 46);
        secp256k1_ec_pubkey_serialize(ctx, data, &publen, &pub, SECP256K1_EC_COMPRESSED);
        datalen = 33;
    }

    // 追加 4 字节 child index (big-endian)
    data[datalen++] = index >> 24;
    data[datalen++] = index >> 16;
    data[datalen++] = index >> 8;
    data[datalen++] = index;

    // HMAC-SHA512(Key = chaincode, Data = 0x00||priv 或 pub33||index)
    unsigned char hmac[64];
    HMAC(EVP_sha512(), parent + 13, 32, data, datalen, hmac, NULL);

    // IL = left 32 bytes → tweak
    // IR = right 32 bytes → child chaincode
    unsigned char il[32];
    memcpy(il, hmac, 32);
    memcpy(child + 13, hmac + 32, 32);  // child chaincode

    // child_priv = parent_priv + IL  (secp256k1 scalar add)
    memcpy(child + 46, parent + 46, 32);
    if (!secp256k1_ec_seckey_tweak_add(ctx, child + 46, il)) return 0;

    // 复制前面的 version, depth, fingerprint, childnum
    memcpy(child, parent, 13);
    child[4]++;                         // depth +1
    // fingerprint = Hash160(compressed parent pubkey) 前4字节
    // 这里简化处理（实际生产代码应重新计算，此处复用简化）

    // 最后4字节 child number
    child[9]  = index >> 24;
    child[10] = index >> 16;
    child[11] = index >> 8;
    child[12] = index;

    return 1;
}

int boss_challenge_balance_02_tests_main(void)
{
    const char *xprv_b58 = "tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*";

    // 步骤1：Base58 解码 tprv
    unsigned char extkey[82];
    size_t extlen;
    if (!DecodeBase58Check(xprv_b58, extkey, &extlen)) { 
        fprintf(stderr, "Base58 decode failed\n");
        return 1;
    }

    if (!verify_checksum(extkey, extlen)) {
        fprintf(stderr, "Invalid checksum\n");
        return 1;
    }

    // 步骤2：准备 secp256k1 上下文
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    if (!ctx) return 1;

    // 步骤3：从 master → m/86'/1'/0'/0 （account 层）
    unsigned char current[78];
    memcpy(current, extkey, 78);  // 78 = version(4)+depth(1)+fp(4)+childnum(4)+chain(32)+0+priv(32)

    uint32_t path[] = {86 | 0x80000000, 1 | 0x80000000, 0 | 0x80000000, 0};
    for (int i = 0; i < 4; i++) {
        if (!derive_child(current, current, path[i], ctx)) {
            fprintf(stderr, "Path derivation failed at %d\n", i);
            goto cleanup;
        }
    }

    // 现在 current 是 m/86'/1'/0'/0 的扩展私钥

    // 步骤4：准备扫描 0~1999 的地址
    const int RANGE = 2000;
    char *addrs_json = malloc(RANGE * 85 + 32);  // 每个地址约 62~65 字符 + 引号+逗号
    if (!addrs_json) goto cleanup;
    strcpy(addrs_json, "[");

    for (int i = 0; i < RANGE; i++) {
        unsigned char child[78];
        if (!derive_child(current, child, (uint32_t)i, ctx)) {
            fprintf(stderr, "Child %d derivation failed\n", i);
            goto cleanup;
        }

        // 步骤5：从 child privkey 得到 internal xonly pubkey
        secp256k1_keypair kp;
        if (!secp256k1_keypair_create(ctx, &kp, child + 46)) {
            fprintf(stderr, "keypair failed %d\n", i);
            goto cleanup;
        }

        secp256k1_xonly_pubkey internal;
        int parity;
        secp256k1_keypair_xonly_pub(ctx, &internal, &parity, &kp);

        // 步骤6：TapTweak (BIP341) - 因为是 key-path-only，所以 tweak = TaggedHash("TapTweak", xonlypub)
        unsigned char tag[32];
        SHA256((const unsigned char*)"TapTweak", 8, tag);
        unsigned char tweak[32];
        // SHA256_CTX s;
        // SHA256_Init(&s);
        // SHA256_Update(&s, tag, 32);
        // SHA256_Update(&s, tag, 32);
        // SHA256_Update(&s, internal.data, 32);
        // SHA256_Final(tweak, &s);
        secp256k1_tagged_sha256(ctx, tweak, tag, sizeof(tag), internal.data, 32);

        // 步骤7：output_pub = internal + tweak
        secp256k1_pubkey output;
        if (!secp256k1_xonly_pubkey_tweak_add(ctx, &output, &internal, tweak)) {
            fprintf(stderr, "tweak add failed %d\n", i);
            goto cleanup;
        }

        // 步骤8：scriptPubKey = 0x01 || 32-byte xonlypub
        unsigned char spk[33] = {0x01};
        memcpy(spk + 1, output.data, 32);

        // 步骤9：Bech32m 编码 → tb1p...
        char addr[85] = "tb1";
        if (!bech32_encode(addr, "tb", spk, 33, 1)) {
            fprintf(stderr, "bech32 failed %d\n", i);
            goto cleanup;
        }

        // 拼接到 json 数组
        char tmp[100];
        snprintf(tmp, sizeof(tmp), "\"addr(%s)\",", addr);
        strcat(addrs_json, tmp);
    }

    // 去掉最后一个逗号，补上 ]
    size_t len = strlen(addrs_json);
    if (len > 1) addrs_json[len-1] = ']';
    else strcat(addrs_json, "]");

    // 步骤10：调用 bitcoin-cli scantxoutset
    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
             "bitcoin-cli -signet -datadir=~/data/0 scantxoutset start '%s'", addrs_json);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        perror("popen failed");
        goto cleanup;
    }

    char buf[16384] = {0};
    fread(buf, 1, sizeof(buf)-1, pipe);
    pclose(pipe);

    // 步骤11：粗暴解析 total_amount （生产环境应使用 json 解析库）
    char *p = strstr(buf, "\"total_amount\":");
    if (p) {
        p += 15;
        while (*p && (*p < '0' || *p > '9') && *p != '.') p++;
        printf("Balance ≈ %s", p);
    } else {
        printf("No total_amount found\n%s\n", buf);
    }

cleanup:
    free(addrs_json);
    secp256k1_context_destroy(ctx);
    return 0;
}