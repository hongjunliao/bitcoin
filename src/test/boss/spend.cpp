/*
 *git@github.com:boss-2026-challenge/signet-wallet-2-spend-hongjunliao.git
 *
 */

// Copyright (c) 2026
//
// Educational example:
// descriptor -> UTXO set -> transaction -> Taproot signature -> raw tx
//
// IMPORTANT:
// Do NOT run this test while bitcoind is using the same chainstate DB.

//boost test: -t boss/spend_p2pkh
//示例输出
// matching UTXOs:
//   txid=a64fab2e2acaf76d46f9e06e82d96222337eb73e74aa44276cacf899176c6c03 vout=526 amount=7144080 sat descriptor_index=222 coinbase=no
//   txid=a4db4b8fcf78d506f7d6fcbd7fd6fbde2a404137d0f44b4c71c58a2ea50efb17 vout=1 amount=828610 sat descriptor_index=202 coinbase=no
//   txid=486049ff894ae8296a1051ae3035ee5a99c0e90bb371e3db306ffa822f3c0e22 vout=127 amount=7144000 sat descriptor_index=223 coinbase=no
//   txid=a95403ac0fd0d9ddf7f9925f597e452f6a54fa20016a2d1b8912219668e84344 vout=0 amount=1013480 sat descriptor_index=111 coinbase=no
//   txid=a32353f04119b37f68ab6291edebe69178196b706934de74af5d302d7467835d vout=409 amount=7144062 sat descriptor_index=226 coinbase=no
//   txid=830b55db534f9a1c24a0c5cb8aa252f40dfba3eff29e65ba147181236816f961 vout=247 amount=7144010 sat descriptor_index=224 coinbase=no
//   txid=3919effb81bb6df4bc1af77f6a110ac4004ed8b02290f986fba402b126d8d28c vout=566 amount=7144693 sat descriptor_index=230 coinbase=no
//   txid=cd72a0c5a903ed2e28bfdc7e1a268583858804b97b6e5babc528715ea8633994 vout=200 amount=7143925 sat descriptor_index=204 coinbase=no
//   txid=be9112d3c1e9febbc4a0ca7771e53cd826434674241357bd0ae5004c353d2698 vout=279 amount=7144104 sat descriptor_index=231 coinbase=no
//   txid=960a2504f60eb49b29825f3ba0bd48206bebbc59dac3046d3abaebaed10c769e vout=640 amount=7144864 sat descriptor_index=229 coinbase=no
//   txid=ddf5abc165e556a0085036ff9b19ce583b26b6a7edd244d87ef087cbad4feca0 vout=0 amount=105804 sat descriptor_index=171 coinbase=no
//   txid=a3988187fcf80010cb1c676e48b6c057ae8a1c6a533c36629981ec4025f4ddb8 vout=1 amount=2498742 sat descriptor_index=214 coinbase=no
//   txid=13d1f5b6c7921f281679543a02c31fb60ac7b04e488e7da3775ec598f3d717c2 vout=595 amount=7144363 sat descriptor_index=227 coinbase=no
//   txid=1bad83e5b96ba3f502f361b7c2ca022d7df339a55ffd507a4ef792a6c6a7f5c3 vout=471 amount=7144193 sat descriptor_index=228 coinbase=no
//   txid=16825ce275bc66eeea0c4494b8d99c6e4c0ea5f5a2d214fd7257f11c550a4fd7 vout=0 amount=705124 sat descriptor_index=77 coinbase=no
//   txid=0bf7615ced757cf4da14c964e2e2ccf72e74227f6884301c752b5899f624d7e1 vout=335 amount=7143915 sat descriptor_index=225 coinbase=no
//   txid=09b9051941499d15ecbe989d14d2545f247c05c4252efe71ede9af45a3a5bfe7 vout=0 amount=2398787 sat descriptor_index=221 coinbase=no
//   txid=d1484d7045a68ebbb1bef5ad2f71c72f5c295fa956554b12a57a0a8d8087d1ec vout=112 amount=7144693 sat descriptor_index=220 coinbase=no
//   txid=d673db156467bffd024d8bb5bf8728cdc0ab7d549f7395e99fc43b07fdd424f4 vout=484 amount=7143937 sat descriptor_index=215 coinbase=no
// Total matching balance: 1.00425386 BTC
// Change index    : 869
// Change address  : tb1pqqyxyn0z0dlnhp6ljawje9f8aems73n2auucc8dr5m384fud9pdqz6apup
//
//
// ============================================================
// FINAL TRANSACTION
// ============================================================
// txid  : fa4f55e6504b65c57f689307f4808ef0aa83fad65fe76ca1ade229c8d23c5994
// wtxid : 542e2f6476065e88a54603fa6149d8279d0d3c76a8dcd18cd4c511f86bfc1400
// hex   :
// 02000000000101036c6c1799f8ac6c2744aa743eb77e332262d9826ee0f9466df7ca2a2eab4fa60e02000000fdffffff02400d0300000000001976a914d524ad80409347c14bd1024e18f0986adb108c3388ac4ff56900000000002251200008624de27b7f3b875f975d2c9527ee770f466aef398c1da3a6e27aa78d285a0140ecb7fea47f61a0b035cceae61daa7037410f02b0cd6794853f3c0606631bf4e64768ccae60f75246a41d72f48eec03719886c26bd962f9a7b2d73c4008b12aef00000000

//示例结果
// bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet decoderawtransaction 02000000000101036c6c1799f8ac6c2744aa743eb77e332262d9826ee0f9466df7ca2a2eab4fa60e02000000fdffffff02400d0300000000001976a914d524ad80409347c14bd1024e18f0986adb108c3388ac4ff56900000000002251200008624de27b7f3b875f975d2c9527ee770f466aef398c1da3a6e27aa78d285a0140ecb7fea47f61a0b035cceae61daa7037410f02b0cd6794853f3c0606631bf4e64768ccae60f75246a41d72f48eec03719886c26bd962f9a7b2d73c4008b12aef00000000
// {
//   "txid": "fa4f55e6504b65c57f689307f4808ef0aa83fad65fe76ca1ade229c8d23c5994",
//   "hash": "542e2f6476065e88a54603fa6149d8279d0d3c76a8dcd18cd4c511f86bfc1400",
//   "version": 2,
//   "size": 196,
//   "vsize": 145,
//   "weight": 580,
//   "locktime": 0,
//   "vin": [
//     {
//       "txid": "a64fab2e2acaf76d46f9e06e82d96222337eb73e74aa44276cacf899176c6c03",
//       "vout": 526,
//       "scriptSig": {
//         "asm": "",
//         "hex": ""
//       },
//       "txinwitness": [
//         "ecb7fea47f61a0b035cceae61daa7037410f02b0cd6794853f3c0606631bf4e64768ccae60f75246a41d72f48eec03719886c26bd962f9a7b2d73c4008b12aef"
//       ],
//       "sequence": 4294967293
//     }
//   ],
//   "vout": [
//     {
//       "value": 0.00200000,
//       "n": 0,
//       "scriptPubKey": {
//         "asm": "OP_DUP OP_HASH160 d524ad80409347c14bd1024e18f0986adb108c33 OP_EQUALVERIFY OP_CHECKSIG",
//         "desc": "addr(mzwxAKy51qBsV61f4C3Rn9SCUBqxDVzUTp)#sn6qzhgr",
//         "hex": "76a914d524ad80409347c14bd1024e18f0986adb108c3388ac",
//         "address": "mzwxAKy51qBsV61f4C3Rn9SCUBqxDVzUTp",
//         "type": "pubkeyhash"
//       }
//     },
//     {
//       "value": 0.06944079,
//       "n": 1,
//       "scriptPubKey": {
//         "asm": "1 0008624de27b7f3b875f975d2c9527ee770f466aef398c1da3a6e27aa78d285a",
//         "desc": "rawtr(0008624de27b7f3b875f975d2c9527ee770f466aef398c1da3a6e27aa78d285a)#fkuxfs0m",
//         "hex": "51200008624de27b7f3b875f975d2c9527ee770f466aef398c1da3a6e27aa78d285a",
//         "address": "tb1pqqyxyn0z0dlnhp6ljawje9f8aems73n2auucc8dr5m384fud9pdqz6apup",
//         "type": "witness_v1_taproot"
//       }
//     }
//   ]
// }


#include <coins.h>
#include <dbwrapper.h>
#include <script/descriptor.h>
#include <script/sign.h>
// #include <script/standard.h>
#include <streams.h>
#include <txdb.h>
#include <util/fs.h>
#include <util/strencodings.h>
#include <util/translation.h>   //bilingual_str
#include "key_io.h"
#include "boss.h"   //SignetSetup

#include <test/util/setup_common.h>

#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <boost/test/unit_test.hpp>
#include "util/moneystr.h"  //FormatMoney
#include "core_io.h"        //EncodeHexTx
#include <test/util/setup_common.h>

#include <functional>
#include <iostream>


const std::string  DESCRIPTOR =
    "tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5";

BOOST_FIXTURE_TEST_SUITE(boss, SignetSetup)


// -t boss/spend_p2pkh -- datadir=/home/jun/bitcoin/datadir/boss to=mzwxAKy51qBsV61f4C3Rn9SCUBqxDVzUTp amount=200000 fee_rate=1
BOOST_AUTO_TEST_CASE(spend_p2pkh) {
    bool ret{false};
    std::string datadir;
    std::string to_addr;
    CAmount amount{200'000};
    CAmount fee_rate{1};
    int range{2000};
    //取得发送参数
    //datadir=/home/jun/bitcoin/bitcoin/signet to=mzwxAKy51qBsV61f4C3Rn9SCUBqxDVzUTp amount=0.002
    {
        for (int i = 0; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
            auto argi = boost::unit_test::framework::master_test_suite().argv[i];

            std::cout << i << " -> " << argi << std::endl;

            if (strncmp(argi, "datadir=", 8) == 0) {
                datadir = argi + 8;
                std::cout << "datadir=" << datadir << std::endl;
                continue;;
            }
            else if (strncmp(argi, "to=", 3) == 0) {
                to_addr = argi + 3;
                std::cout << "to=" << to_addr << std::endl;
                continue;;
            }
            else if (strncmp(argi, "range=", 6) == 0) {
                range = atoi((argi + 6));
                std::cout << "range=" << range << std::endl;
                continue;;
            }
            else if (strncmp(argi, "fee_rate=", 9) == 0) {
                fee_rate = strtol((argi + 9), nullptr, 10);
                std::cout << "fee_rate=" << fee_rate << std::endl;
                continue;;
            }
            else if (strncmp(argi, "amount=", 7) == 0) {
                amount = strtol(argi + 7, nullptr, 10);
                std::cout << "amount=" << amount << std::endl;
                continue;;
            }
        }
    }
    //验证参数
    const CTxDestination destination = DecodeDestination(to_addr);
    BOOST_REQUIRE_MESSAGE(
        IsValidDestination(destination),
        "Invalid destination address: " << to_addr);


    //解析descriptor字符串，得到 Descriptor
    FlatSigningProvider provider;
    std::string err;
    auto desc_vec = Parse(DESCRIPTOR, provider, err, true);
    brequire(desc_vec.size() >= 1);
    auto& desc = desc_vec[0];

    //展开descriptor
    std::map<CScript, std::tuple<int,int>> scriptsmap;
    {
        for (int i = 0; i < range; ++i) {
            std::vector<CScript> scripts;
            FlatSigningProvider expanded_provider;

            if (!desc->Expand(i,provider,scripts,expanded_provider)) {

                BOOST_TEST_MESSAGE("descriptor Expand failed at index " << i);
                continue;
            }

            for (const CScript& script : scripts) {
                scriptsmap.emplace(script, std::make_tuple(i, 0));
            }
        }
    }

    std::map<COutPoint, Coin> selected_utxos;
    CAmount utxo_amount{0};
    CAmount total_available = 0;

    CMutableTransaction mtx;
    FlatSigningProvider signing_provider;

    //查询可用UTXO
    // bitcoind must NOT be running because LevelDB locks chainstate
    {
        auto cpath{std::filesystem::path{datadir} / "signet" / "chainstate"};
        bcheck(fs::exists(cpath));

        DBParams db_params{
            .path = cpath,
            .cache_bytes = 64_MiB,
            .memory_only = false,
            .wipe_data = false,
            .obfuscate = true,
        };

        CCoinsViewDB coinsdb{
            std::move(db_params),
            CoinsViewOptions{}
        };

        // 5. Print UTXOs
        std::cout << "matching UTXOs:\n";

        auto cursor = coinsdb.Cursor();
        while (cursor->Valid()) {
            COutPoint outpoint;
            Coin coin;

            if (!(cursor->GetKey(outpoint) && cursor->GetValue(coin)) || coin.IsSpent()) {
                cursor->Next();
                continue;
            }

            // IMPORTANT:
            //
            // chainstate 本身只知道 UTXO，不知道 mempool 中是否已经有
            // 某个交易正在花费这个 outpoint。
            //
            // 因此这里不能检查 mempool。
            //
            // 另外，为了简单起见，这个学习版本跳过 coinbase UTXO。
            // 这样就不需要加载 block index 来判断 COINBASE_MATURITY。
            if (coin.IsCoinBase()) {
                cursor->Next();
                continue;
            }

            if (scriptsmap.contains(coin.out.scriptPubKey)) {

                auto& tu = scriptsmap[coin.out.scriptPubKey];

                std::cout
                    << "  txid="
                    << outpoint.hash.GetHex()
                    << " vout="
                    << outpoint.n
                    << " amount="
                    << coin.out.nValue
                    << " sat"
                    << " descriptor_index="
                    << std::get<0>(tu)
                    << " coinbase="
                    << (coin.IsCoinBase() ? "yes" : "no")
                    << "\n";

                total_available += coin.out.nValue;


                std::get<1>(tu) = 1;

                //从可用UTXO中选UTXO，直到可发送的金额足够大
                if (utxo_amount < fee_rate + amount) {

                    std::vector<CScript> scripts;
                    if (!desc->Expand(std::get<0>(tu), provider,scripts,signing_provider)) {

                        BOOST_TEST_MESSAGE("descriptor Expand failed at index " << std::get<0>(tu));
                        continue;
                    }

                    utxo_amount += coin.out.nValue;

                    //加入本次交易
                    CTxIn txin;
                    txin.prevout = outpoint;
                    //
                    // Enable BIP125 RBF.
                    //
                    // 0xfffffffd = MAX_SEQUENCE - 2
                    //
                    // This is not necessary for signing, but is convenient during testing.
                    //
                    txin.nSequence = 0xfffffffd;
                    mtx.vin.push_back(txin);
                    desc->ExpandPrivate(std::get<0>(tu), provider, signing_provider);

                    selected_utxos[outpoint] = coin;
                }
            }

            cursor->Next();
        }
    }

    std::cout
        << "Total matching balance: "
        << FormatMoney(total_available)
        << " BTC\n";

    // Output #0:
    //     mzwxAKy51qBsV61f4C3Rn9SCUBqxDVzUTp
    //     0.002 BTC
    //
    mtx.vout.emplace_back(amount, GetScriptForDestination(destination));
    // Output #1:
    //change,找零
    auto change = utxo_amount - (fee_rate + amount);
    for (auto const& [script, tu] : scriptsmap) {
        if (std::get<1>(tu) == 0) {
            auto change_index = std::get<0>(tu);
            CTxDestination change_destination;

            if (ExtractDestination(script, change_destination)) {
                std::cout
                << "Change index    : "
                        << change_index
                        << "\n"
                        << "Change address  : "
                        << EncodeDestination(change_destination)
                        << "\n"
                        << std::endl;

                mtx.vout.emplace_back(change,script);
                break;
            }
        }
    }


    //签名交易
    SignOptions sign_options{
        .sighash_type = SIGHASH_DEFAULT
    };

    std::map<int, bilingual_str> input_errors;
    ret = SignTransaction(mtx, &signing_provider, selected_utxos, sign_options, input_errors);
    bcheck(ret);
    if (!input_errors.empty()) {
        for (const auto &[index, error_msg]: input_errors) {
            std::cerr
                    << "Input "
                    << index
                    << " signing error: "
                    << error_msg.original
                    << std::endl;
        }
    }

    //输出最终交易
    {
        const CTransaction final_tx(mtx);

        const std::string tx_hex =
                EncodeHexTx(final_tx);

        std::cout
                << "\n"
                << "============================================================\n"
                << "FINAL TRANSACTION\n"
                << "============================================================\n"
                << "txid  : " << final_tx.GetHash().ToString() << "\n"
                << "wtxid : " << final_tx.GetWitnessHash().ToString() << "\n"
                << "hex   :\n"
                << tx_hex << "\n"
                << "============================================================\n"
                << "\n"
                << "Broadcast after starting bitcoind:\n"
                << "\n"
                << "bitcoin-cli -signet -datadir=" << datadir << " sendrawtransaction "
                << tx_hex << "\n"
                << std::endl;
    }
}


BOOST_AUTO_TEST_SUITE_END()