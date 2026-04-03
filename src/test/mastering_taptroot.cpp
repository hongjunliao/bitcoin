/*! 
 * mastering taproot 
 */
 #include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h> //TestingSetup
#include <base58.h>                 //EncodeBase58Check
#include <vector>
#include <key_io.h>                    //EncodeSecret
#include <pubkey.h>                    //CPubKey, XOnlyPubKey

 BOOST_FIXTURE_TEST_SUITE(mastering_taproot, TestingSetup)

 BOOST_AUTO_TEST_CASE(pri_key)
{
    // private key
    CKey k;
    BOOST_CHECK(!k.IsValid());
    auto hex = ParseHex("e9873d79c6d87dc0fb6a5778633389dfa5c32fa27f99b5199abf2f9848ee0289");
    k.Set(hex.begin(), hex.end(), true);
    // k.MakeNewKey(true);
    BOOST_CHECK(k.IsValid());

    printf("Private Key (HEX): %s\n", HexStr(std::span{k.begin(), k.end()}).c_str());
    BOOST_CHECK(HexStr(std::span{k.begin(), k.end()}) == "e9873d79c6d87dc0fb6a5778633389dfa5c32fa27f99b5199abf2f9848ee0289");

    printf("Private Key (WIF): %s\n", EncodeSecret(k).c_str());
    // BOOST_CHECK(EncodeSecret(k) == "L1aW4aubDFB7yfras2S1mN3bqg9w3KmCPSM3Qh4rQG9E1e84n5Bd");

    //WIF format of private key
    auto dk = DecodeSecret("L1aW4aubDFB7yfras2S1mN3bqg9w3KmCPSM3Qh4rQG9E1e84n5Bd"); 
    // BOOST_CHECK(dk.IsValid());
    // BOOST_CHECK(k == dk);

    //public key
    auto pk = k.GetPubKey(), cpk = pk;
    printf("Public Key (HEX): %s\n", HexStr(std::span{pk.begin(), pk.end()}).c_str());
    pk.Decompress();
    printf("Decompressed Public Key (HEX): %s\n", HexStr(std::span{pk.begin(), pk.end()}).c_str());

    //x-only public key
    XOnlyPubKey xpk{pk};
    printf("Xonly Public Key (HEX): %s\n", HexStr(std::span{xpk.begin(), xpk.end()}).c_str());

    // key to address
        //p2pk
    PubKeyDestination p2pk{cpk};
    auto addr_p2pk = EncodeDestination(p2pk);
    printf("P2PK Address: %s\n", addr_p2pk.c_str());
     CScript scriptPubKey = CScript() << cpk << OP_CHECKSIG;
     printf("P2PK ScriptPubKey: %s\n", HexStr(scriptPubKey).c_str());
        //p2pkh
     auto addr_p2pkh = EncodeDestination(PKHash(cpk));
     printf("P2PKH Address: %s\n", addr_p2pkh.c_str());
        //p2wpkh
     auto addr_p2wpkh = EncodeDestination(WitnessV0KeyHash(cpk));
     printf("P2WPKH Address (SegWit Native): %s\n", addr_p2wpkh.c_str());
        //Nested SegWit（P2SH-P2WPKH）地址生成
    auto addr_p2sh_p2wpkh = EncodeDestination(ScriptHash(GetScriptForDestination(WitnessV0KeyHash(cpk))));
    printf("P2SH-P2WPKH Address (SegWit P2SH): %s\n", addr_p2sh_p2wpkh.c_str());
        //p2tr
     auto addr_p2tr = EncodeDestination(WitnessV1Taproot(xpk));
     printf("P2TR Address: %s\n", addr_p2tr.c_str());

}

BOOST_AUTO_TEST_SUITE_END()
