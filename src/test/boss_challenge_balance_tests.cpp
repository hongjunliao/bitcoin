/*!
 * 参考答案示例:
 * bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet-wallet-1-balance-hongjunliao/data/0 scantxoutset start '[{"desc":"tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5","range":2000}]'
 * 输出:
	{
	  "success": true,
	  "txouts": 28669,
	  "height": 1462,
	  "bestblock": "00000085fe74ab6ea2db65da3cd5786ec85b68f3f25b27b5b0e6abe132ea8e75",
	  "unspents": [
		...
		{
		  "txid": "d673db156467bffd024d8bb5bf8728cdc0ab7d549f7395e99fc43b07fdd424f4",
		  "vout": 484,
		  "scriptPubKey": "5120ac551df7866f4715dcbde804ca3b1960d204cb2f3c1542b72041d85828e804be",
		  "desc": "tr([019edbea/86h/1h/0h/0/215]a8e948725f5cadd9df8276566fe43b4e0de8f7f8e40d31bd893fd0b6d2637730)#0fywrl2q",
		  "amount": 0.07143937,
		  "coinbase": false,
		  "height": 285,
		  "blockhash": "000000163026df0a6647af21305a68923d8d0fbde182f50dddbc0e834136fc82",
		  "confirmations": 1178
		}
	  ],
	  "total_amount": 1.00425386
	}

 * DecodeExtKey()，和CextKey.Derive()
 * */
/////////////////////////////////////////////////////////////////////////////////////

#include <string>

#include <base58.h>
#include <test/util/json.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <util/vector.h>

#include <univalue.h>
#include <script/descriptor.h>

#include <boost/test/unit_test.hpp>

#include <key.h>
#include <key_io.h>
#include <serialize.h>
#include <hash.h>
#include <pubkey.h>                 // XOnlyPubKey, CPubKey
#include "secp256k1.h"
#include "secp256k1_extrakeys.h"

/////////////////////////////////////////////////////////////////////////////////////

using namespace std::literals;
using namespace util::hex_literals;

const std::string BOSS_DESC =
"tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5";

struct SignetSetup : public TestingSetup {
	SignetSetup()
        : TestingSetup{ChainType::SIGNET} {}
};

/////////////////////////////////////////////////////////////////////////////////////

// 全局 secp256k1 上下文（Bitcoin Core 通常使用静态上下文）

// Tagged Hash 辅助函数（Bitcoin Core 风格）
uint256 ComputeTaggedHash(const std::string& tag, const std::vector<unsigned char>& data) {
    uint256 hash;
    CSHA256 hash_ctx;

    // tag hash 前缀
    std::vector<unsigned char> tag_hash(32);
    hash_ctx.Write((const unsigned char*)tag.data(), tag.size());
    hash_ctx.Finalize(tag_hash.data());

    // 双重 SHA256(tag) || 双重 SHA256(tag) || data
    hash_ctx.Reset();
    hash_ctx.Write(tag_hash.data(), 32);
    hash_ctx.Write(tag_hash.data(), 32);
    hash_ctx.Write(data.data(), data.size());
    hash_ctx.Finalize(hash.begin());

    return hash;
}

// 计算单个 internal key 的 tweaked output key（符合 BIP-341 unspendable path）
bool ComputeTweakedOutputKey(
    const XOnlyPubKey& internal_xonly,
    XOnlyPubKey& output_xonly)
{
    // 1. 准备 unspendable leaf script（示例：OP_RETURN + 32字节 NUMS point）
    // 这里使用 BIP-341 示例中的一个常见 NUMS point（无对应私钥）
    static const unsigned char nums_point[32] = {
        0x50, 0x92, 0x9b, 0x74, 0xc1, 0xa0, 0x49, 0x54,
        0xb7, 0x8b, 0x4b, 0x60, 0x35, 0xe9, 0x7a, 0x5e,
        0x07, 0x8a, 0x5a, 0x0f, 0x28, 0xec, 0x96, 0xd5,
        0x47, 0xbf, 0xee, 0x9a, 0xce, 0x80, 0x3a, 0xc0
    };

    std::vector<unsigned char> leaf_script;
    leaf_script.push_back(0x6a);                     // OP_RETURN
    leaf_script.push_back(0x20);                     // PUSH32
    leaf_script.insert(leaf_script.end(), nums_point, nums_point + 32);

    // 2. 计算 leaf hash = TaggedHash("TapLeaf", 0xc0 || compact_size(script) || script)
    std::vector<unsigned char> leaf_data;
    leaf_data.push_back(0xc0);  // 当前 leaf version

    // compact size of script length
    size_t len = leaf_script.size();
    if (len < 253) {
        leaf_data.push_back(static_cast<unsigned char>(len));
    } else {
        // 实际项目中很少超过253字节，这里简化
        return false;
    }
    leaf_data.insert(leaf_data.end(), leaf_script.begin(), leaf_script.end());

    uint256 leaf_hash = ComputeTaggedHash("TapLeaf", leaf_data);

    // 3. 因为只有一个 leaf，merkle_root = leaf_hash
    uint256 merkle_root = leaf_hash;

    // 4. 计算 tweak = TaggedHash("TapTweak", internal_xonly(32) || merkle_root(32))
    std::vector<unsigned char> tweak_input;
    tweak_input.insert(tweak_input.end(), internal_xonly.begin(), internal_xonly.end());
    tweak_input.insert(tweak_input.end(), merkle_root.begin(), merkle_root.end());

    uint256 tweak = ComputeTaggedHash("TapTweak", tweak_input);

    // 5. 使用 secp256k1_xonly_pubkey_tweak_add 计算 tweaked pubkey
    secp256k1_xonly_pubkey internal_secp;
    if (!secp256k1_xonly_pubkey_parse(secp256k1_context_static, &internal_secp, internal_xonly.data())) {
        return false;
    }

    secp256k1_pubkey output_secp;
    if (!secp256k1_xonly_pubkey_tweak_add(
            secp256k1_context_static,
            &output_secp,
            &internal_secp,
            tweak.begin())) {
        return false;
    }

    // 6. 转回 XOnlyPubKey（32字节）
    secp256k1_xonly_pubkey output_xonly_secp;
    int parity;
    if (!secp256k1_xonly_pubkey_from_pubkey(
            secp256k1_context_static,
            &output_xonly_secp,
            &parity,
            &output_secp)) {
        return false;
    }

    std::array<unsigned char, 32> output_bytes;
    secp256k1_xonly_pubkey_serialize(secp256k1_context_static, output_bytes.data(), &output_xonly_secp);
    output_xonly = XOnlyPubKey(output_bytes);

    return true;
}

// 批量处理你的 2000 个 internal keys
void ProcessAllKeys(const std::vector<XOnlyPubKey>& internal_keys) {
    std::vector<XOnlyPubKey> tweaked_keys;
    tweaked_keys.reserve(internal_keys.size());

    for (size_t i = 0; i < internal_keys.size(); ++i) {
        XOnlyPubKey tweaked;
        if (ComputeTweakedOutputKey(internal_keys[i], tweaked)) {
            tweaked_keys.push_back(tweaked);

            // 调试输出前几个
            if (i < 5) {
                std::cout << "Index " << i << ":\n";
//                std::cout << "  Internal: " << HexStr(internal_keys[i]) << "\n";
//                std::cout << "  Tweaked : " << HexStr(tweaked) << "\n\n";
            }
        } else {
            std::cerr << "Failed to tweak key at index " << i << std::endl;
        }
    }

    std::cout << "Successfully tweaked " << tweaked_keys.size() << " keys.\n";
}

BOOST_FIXTURE_TEST_SUITE(boss_challenge_balance_tests, SignetSetup)
BOOST_AUTO_TEST_CASE(boss_challenge_balance_02)
{
	int i;
    CExtKey extkey = DecodeExtKey("tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ");
    BOOST_CHECK(extkey.key.IsValid());
    std::vector<XOnlyPubKey> xonly_pubkeys;
    for(i = 0; i < 2000; ++i){
    	CExtKey child;
        BOOST_CHECK(extkey.Derive(child, i));
		CExtPubKey pubkeyNew = child.Neuter();
        XOnlyPubKey xonly_pub(pubkeyNew.pubkey);
        xonly_pubkeys.push_back(xonly_pub);
    }
    ProcessAllKeys(xonly_pubkeys);
}
BOOST_AUTO_TEST_CASE(boss_challenge_balance_03)
{
	extern int boss_challenge_balance_02_tests_main();
	// boss_challenge_balance_02_tests_main();
}
BOOST_AUTO_TEST_CASE(boss_challenge_balance)
{
	int i;
    FlatSigningProvider keys_priv, keys_pub;
    std::string error;
    CExtKey extkey;

    std::vector<std::unique_ptr<Descriptor>> parse_privs;
    parse_privs = Parse(BOSS_DESC, keys_priv, error, true);
    BOOST_CHECK_MESSAGE(!parse_privs.empty(), error);
//    auto& key = parse_privs.at(0);
    BOOST_CHECK(keys_priv.keys.empty());
    auto  key = keys_priv.keys.begin();
    BOOST_CHECK(key->second.IsValid());
    // extkey = key->second;

//    auto& parse_pub = parse_pubs.at(desc_index);

    extkey = DecodeExtKey("tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ");
    BOOST_CHECK(extkey.key.IsValid());
    // Derive new keys
    CExtKey keyNew;
    BOOST_CHECK(!keyNew.key.IsValid());
    BOOST_CHECK(extkey.Derive(keyNew, 2000));
    BOOST_CHECK(keyNew.key.IsValid());

    std::vector<XOnlyPubKey> xonly_pubkeys;
    for(i = 0; i < 2000; ++i){
    	CExtKey child;
        BOOST_CHECK(extkey.Derive(child, i));
		CExtPubKey pubkeyNew = child.Neuter();
        XOnlyPubKey xonly_pub(pubkeyNew.pubkey);
        xonly_pubkeys.push_back(xonly_pub);

		// 方式2：显式构造 WitnessV1Taproot（更安全，类型更明确）
		WitnessV1Taproot wit1(xonly_pub);
//		p2tr_witness_programs.push_back(wit1.program);  // 也是 32 字节
//            p2tr_witness_programs.push_back(program);
    }
}
BOOST_AUTO_TEST_SUITE_END()
