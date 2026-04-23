/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <boost/test/unit_test.hpp>
#include <secp256k1.h>
#include <sqlite3.h>
#include "hp/sdsinc.h"
#include "btc_net.h"
#include "hp/hp_log.h"

// Base58 编码表
static const char *base58_alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// 16 进制转储函数
void hex_dump(unsigned char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

// Base58Check 编码
void base58check_encode(const unsigned char *data, size_t data_len, char *output, size_t output_len) {
    // 计算 SHA256 两次
    unsigned char hash1[SHA256_DIGEST_LENGTH];
    unsigned char hash2[SHA256_DIGEST_LENGTH];
    SHA256(data, data_len, hash1);
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);

    // 拼接数据 + 前 4 字节校验码
    auto temp = (unsigned char *)malloc(data_len + 4);
    memcpy(temp, data, data_len);
    memcpy(temp + data_len, hash2, 4);

    // Base58 编码
    size_t i, j;
    unsigned long long num = 0;
    char *out = output;
    int leading_zeros = 0;

    // 计算前导零
    for (i = 0; i < data_len + 4 && temp[i] == 0; i++) {
        leading_zeros++;
    }

    // 转换为 Base58
//    for (i =  makeshift:
    auto buf = (unsigned char *)malloc(1024);
    size_t buf_len = 0;
    for (i = 0; i < data_len + 4; i++) {
        num = num * 256 + temp[i];
        while (num >= 58) {
            buf[buf_len++] = base58_alphabet[num % 58];
            num /= 58;
        }
    }

    // 反转并添加前导 1
    for (i = 0; i < leading_zeros; i++) {
        *out++ = '1';
    }
    for (i = buf_len - 1; i >= 0; i--) {
        *out++ = buf[i];
    }
    *out = '\0';

    free(buf);
    free(temp);
}

static int SetPragma(sqlite3* db, const std::string& key, const std::string& value, const std::string& err_msg)
{
    auto s = sdscatfmt(sdsempty(), "PRAGMA %s = %s", key.c_str(), value.c_str());
    int rc = sqlite3_exec(db, s, nullptr, nullptr, nullptr);
    sdsfree(s);
    return rc;
}

//static std::optional<int> ReadPragmaInteger(sqlite3* db, const std::string& key, const std::string& description, bilingual_str& error)
//{
//    std::string stmt_text = strprintf("PRAGMA %s", key);
//    sqlite3_stmt* pragma_read_stmt{nullptr};
//    int ret = sqlite3_prepare_v2(db, stmt_text.c_str(), -1, &pragma_read_stmt, nullptr);
//    if (ret != SQLITE_OK) {
//        sqlite3_finalize(pragma_read_stmt);
//        error = Untranslated(strprintf("SQLiteDatabase: Failed to prepare the statement to fetch %s: %s", description, sqlite3_errstr(ret)));
//        return std::nullopt;
//    }
//    ret = sqlite3_step(pragma_read_stmt);
//    if (ret != SQLITE_ROW) {
//        sqlite3_finalize(pragma_read_stmt);
//        error = Untranslated(strprintf("SQLiteDatabase: Failed to fetch %s: %s", description, sqlite3_errstr(ret)));
//        return std::nullopt;
//    }
//    int result = sqlite3_column_int(pragma_read_stmt, 0);
//    sqlite3_finalize(pragma_read_stmt);
//    return result;
//}
//
//int wallet_verify(sqlite3 * m_db)
//{
//    assert(m_db);
//
//    // Check the application ID matches our network magic
//    auto read_result = ReadPragmaInteger(m_db, "application_id", "the application id", error);
//    if (!read_result.has_value()) return false;
//    uint32_t app_id = static_cast<uint32_t>(read_result.value());
//    uint32_t net_magic = ReadBE32(Params().MessageStart().data());
//    if (app_id != net_magic) {
//        error = strprintf(_("SQLiteDatabase: Unexpected application id. Expected %u, got %u"), net_magic, app_id);
//        return false;
//    }
//
//    // Check our schema version
//    read_result = ReadPragmaInteger(m_db, "user_version", "sqlite wallet schema version", error);
//    if (!read_result.has_value()) return false;
//    int32_t user_ver = read_result.value();
//    if (user_ver != WALLET_SCHEMA_VERSION) {
//        error = strprintf(_("SQLiteDatabase: Unknown sqlite wallet schema version %d. Only version %d is supported"), user_ver, WALLET_SCHEMA_VERSION);
//        return false;
//    }
//
//    sqlite3_stmt* stmt{nullptr};
//    int ret = sqlite3_prepare_v2(m_db, "PRAGMA integrity_check", -1, &stmt, nullptr);
//    if (ret != SQLITE_OK) {
//        sqlite3_finalize(stmt);
//        error = strprintf(_("SQLiteDatabase: Failed to prepare statement to verify database: %s"), sqlite3_errstr(ret));
//        return false;
//    }
//    while (true) {
//        ret = sqlite3_step(stmt);
//        if (ret == SQLITE_DONE) {
//            break;
//        }
//        if (ret != SQLITE_ROW) {
//            error = strprintf(_("SQLiteDatabase: Failed to execute statement to verify database: %s"), sqlite3_errstr(ret));
//            break;
//        }
//        const char* msg = (const char*)sqlite3_column_text(stmt, 0);
//        if (!msg) {
//            error = strprintf(_("SQLiteDatabase: Failed to read database verification error: %s"), sqlite3_errstr(ret));
//            break;
//        }
//        std::string str_msg(msg);
//        if (str_msg == "ok") {
//            continue;
//        }
//        if (error.empty()) {
//            error = _("Failed to verify database") + Untranslated("\n");
//        }
//        error += Untranslated(strprintf("%s\n", str_msg));
//    }
//    sqlite3_finalize(stmt);
//    return error.empty();
//}
int wallet_open_sqlit3(sqlite3* db, std::string filepath)
{
	int flags = SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	// Setup logging
	int ret = sqlite3_config(SQLITE_CONFIG_LOG, /*ErrorLogCallback*/0, nullptr);
	// Force serialized threading mode
	ret = SQLITE_OK == ret? sqlite3_config(SQLITE_CONFIG_SERIALIZED) : ret;
	ret = SQLITE_OK == ret? sqlite3_initialize() : ret; // This is a no-op if sqlite3 is already initialized
	ret = SQLITE_OK == ret? sqlite3_open_v2(filepath.c_str(), &db, flags, nullptr) : ret;
	ret = SQLITE_OK == ret? sqlite3_extended_result_codes(db, 1) : ret;
	ret = SQLITE_OK == ret? (sqlite3_db_readonly(db, "main") == 0? SQLITE_OK : ret) : ret;

	// Acquire an exclusive lock on the database
	// First change the locking mode to exclusive
	ret = SQLITE_OK == ret? SetPragma(db, "locking_mode", "exclusive", "Unable to change database locking mode to exclusive") : ret;
	// Now begin a transaction to acquire the exclusive lock. This lock won't be released until we close because of the exclusive locking mode.
	ret = SQLITE_OK == ret? sqlite3_exec(db, "BEGIN EXCLUSIVE TRANSACTION", nullptr,nullptr, nullptr) : ret;
	ret = SQLITE_OK == ret? sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) : ret;

	// Enable fullfsync for the platforms that use it
	ret = SQLITE_OK == ret? SetPragma(db, "fullfsync", "true", "Failed to enable fullfsync") : ret;

//	if (m_use_unsafe_sync) {
//		// Use normal synchronous mode for the journal
//		LogPrintf(
//				"WARNING SQLite is configured to not wait for data to be flushed to disk. Data loss and corruption may occur.\n");
//		SetPragma(m_db, "synchronous", "OFF", "Failed to set synchronous mode to OFF");
//	}

	// Make the table for our key-value pairs
	// First check that the main table exists
	sqlite3_stmt *check_main_stmt { nullptr };
	ret = SQLITE_OK == ret? sqlite3_prepare_v2(db,
			"SELECT name FROM sqlite_master WHERE type='table' AND name='main'", -1,
			&check_main_stmt, nullptr) : ret;
	int step = SQLITE_OK == ret? sqlite3_step(check_main_stmt) : SQLITE_ERROR;
	if(SQLITE_ERROR != step) {
		ret = sqlite3_finalize(check_main_stmt);
		if(step == SQLITE_DONE){
			ret = SQLITE_OK == ret? sqlite3_exec(db,
							"CREATE TABLE main(key BLOB PRIMARY KEY NOT NULL, value BLOB NOT NULL)",
							nullptr, nullptr, nullptr) : ret;
			// Set the application id
//			uint32_t app_id = ReadBE32(Params().MessageStart().data());
			SetPragma(db, "application_id", "0x1c163f28", "Failed to set the application id");

			// Set the user version
			SetPragma(db, "user_version", /*WALLET_SCHEMA_VERSION*/"0",
					"Failed to set the wallet schema version");
		}
		else if(step == SQLITE_ROW) {}
		else ret = SQLITE_ERROR;
	}
	else ret = SQLITE_ERROR;

	//SQLiteBatch::ReadKey
	return ret == SQLITE_OK? 0 : -1;
}
int wallet_main() {
    // 初始化 secp256k1 上下文
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    unsigned char private_key[32];
    unsigned char public_key[65];
    unsigned char compressed_pubkey[33];
    unsigned char hash[RIPEMD160_DIGEST_LENGTH];
    unsigned char address_data[25];
    char address[35];

    // 生成随机私钥
    FILE *frand = fopen("/dev/urandom", "r");
    fread(private_key, 32, 1, frand);
    fclose(frand);

    // 验证私钥
    if (!secp256k1_ec_seckey_verify(ctx, private_key)) {
        printf("Invalid private key\n");
        return 1;
    }

    // 生成公钥
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, private_key)) {
        printf("Failed to create public key\n");
        return 1;
    }

    // 压缩公钥
    size_t pubkey_len = 65;
    secp256k1_ec_pubkey_serialize(ctx, public_key, &pubkey_len, &pubkey, SECP256K1_EC_UNCOMPRESSED);
    pubkey_len = 33;
    secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey, &pubkey_len, &pubkey, SECP256K1_EC_COMPRESSED);

    // SHA256 哈希
    unsigned char sha256_hash[SHA256_DIGEST_LENGTH];
    SHA256(compressed_pubkey, 33, sha256_hash);

    // RIPEMD160 哈希
    RIPEMD160(sha256_hash, SHA256_DIGEST_LENGTH, hash);

    // 构造地址数据（testnet 前缀 0x6F）
    address_data[0] = 0x6F; // Testnet P2PKH 前缀
    memcpy(address_data + 1, hash, RIPEMD160_DIGEST_LENGTH);

    // Base58Check 编码
    base58check_encode(address_data, 21, address, sizeof(address));

    // 输出结果
    printf("Private Key: ");
    hex_dump(private_key, 32);
    printf("Public Key (compressed): ");
    hex_dump(compressed_pubkey, 33);
    printf("Testnet Address: %s\n", address);

    // 清理
    secp256k1_context_destroy(ctx);
    return 0;
}

BOOST_AUTO_TEST_SUITE(wallet)

BOOST_AUTO_TEST_CASE(sqlite_wallet) {
	return;
	sqlite3* db{0};
	assert(wallet_open_sqlit3(db, "test/wallet.dat") == 0);
	assert(!db);
	sqlite3_close(db);
	//SQLiteBatch::TxnBegin
	assert(wallet_main() == 0);
}

BOOST_AUTO_TEST_SUITE_END()

