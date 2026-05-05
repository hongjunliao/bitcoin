/*! 
 * mastering taproot 
 */
 #include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h> //TestingSetup
#include <base58.h>                 //EncodeBase58Check
#include <vector>
#include <key_io.h>                    //EncodeSecret
#include <pubkey.h>                    //CPubKey, XOnlyPubKey
#include <core_io.h>                 //EncodeHexTx
#include "script/interpreter.h"      //SignatureHash
#include <policy/policy.h>           //GetVirtualTransactionSize
#include <serialize.h>               //GetSerializeSize
#include <node/protocol_version.h>             //PROTOCOL_VERSION    
#include <script/sign.h>            //ProduceSignature

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

#define bcheck BOOST_CHECK 
#define brequire BOOST_REQUIRE
#define bequal BOOST_CHECK_EQUAL


struct TestnetSetup : public TestingSetup {
	TestnetSetup()
        : TestingSetup{ChainType::TESTNET} {}
};

 BOOST_FIXTURE_TEST_SUITE(mastering_taproot, TestnetSetup)

 BOOST_AUTO_TEST_CASE(chapter_01)
{
    std::cout << "=== Chapter 1: Key Generation and Address Creation ===" << std::endl;
    // private key
    CKey k;
    bcheck(!k.IsValid());
    auto hex = ParseHex("e9873d79c6d87dc0fb6a5778633389dfa5c32fa27f99b5199abf2f9848ee0289");
    k.Set(hex.begin(), hex.end(), true);
    // k.MakeNewKey(true);
    bcheck(k.IsValid());

    printf("Private Key (HEX): %s\n", HexStr(std::span{k.begin(), k.end()}).c_str());
    bcheck(HexStr(std::span{k.begin(), k.end()}) == "e9873d79c6d87dc0fb6a5778633389dfa5c32fa27f99b5199abf2f9848ee0289");

    printf("Private Key (WIF): %s\n", EncodeSecret(k).c_str());
    // bcheck(EncodeSecret(k) == "L1aW4aubDFB7yfras2S1mN3bqg9w3KmCPSM3Qh4rQG9E1e84n5Bd");

    //WIF format of private key
    auto dk = DecodeSecret("L1aW4aubDFB7yfras2S1mN3bqg9w3KmCPSM3Qh4rQG9E1e84n5Bd"); 
    // bcheck(dk.IsValid());
    // bcheck(k == dk);

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

     std::cout << "end of chapter 1________________________________________________" << std::endl;
}

BOOST_AUTO_TEST_CASE(chapter_02)
{
    std::cout << "=== Chapter 2:Bitcoin Script and P2PKH Fundamentals ===" << std::endl;
    // -- SETUP --
    // Testnet WIF private key
    std::string wif_str = "cPeon9fBsW2BxwJTALj3hGzh9vm8C52Uqsce7MzXGS1iFJkPF4AT";
    // CKey key;
    // key.Set(key.DecodeBase58Check(wif_str), wif_str[0] == 'c' || wif_str[0] == '9'); // testnet/compressed
    CKey key = DecodeSecret(wif_str);
    bcheck(key.IsValid());

    CPubKey pubkey = key.GetPubKey();

    // // Sender address: myYHJtG3cyoRseuTwvViGHgP2efAvZkYa4
    auto from_address_str = "myYHJtG3cyoRseuTwvViGHgP2efAvZkYa4";
    bcheck(IsValidDestinationString(from_address_str));
    CTxDestination from_dest = DecodeDestination(from_address_str);

    // // Receiver SegWit address: tb1qckeg66a6jx3xjw5mrpmte5ujjv3cjrajtvm9r4
    auto to_address_str = "tb1qckeg66a6jx3xjw5mrpmte5ujjv3cjrajtvm9r4";
    bcheck(IsValidDestinationString(to_address_str));
    CTxDestination to_dest = DecodeDestination(to_address_str);

    bcheck(IsValidDestination(from_dest));
    bcheck(IsValidDestination(to_dest));

    std::cout << "Sender Legacy Address: " << EncodeDestination(from_dest) << std::endl;
    std::cout << "Receiver SegWit Address: " << EncodeDestination(to_dest) << std::endl;

    // -- CREATE TRANSACTION INPUT --
    Txid txid = Txid::FromUint256(uint256("34b90a15d0a9ec9ff3d7bed2536533c73278a9559391cb8c9778b7e7141806f7"));
    uint32_t vout = 1;

    // -- AMOUNTS --
    // total input:  0.00029606 BTC
    // amount to send: 0.00029400 BTC
    // fee: 0.00000206 BTC
    CAmount total_input = 29606;     // satoshis
    CAmount send_amount = 29400;      // satoshis
    // CAmount fee = total_input - send_amount;

    // -- CONSTRUCT UNSIGNED TRANSACTION --
    CMutableTransaction mtx;

    mtx.vin.emplace_back(COutPoint(txid, vout));
    // Output to recipient
    CScript to_script_pubkey = GetScriptForDestination(to_dest);
    mtx.vout.emplace_back(send_amount, to_script_pubkey);

    // Print unsigned tx hex
    std::cout << "Unsigned tx: " << EncodeHexTx(CTransaction{mtx}) << std::endl;

    // -- SIGN INPUT --
    CScript from_script_pubkey = GetScriptForDestination(from_dest);
    uint256 sighash = SignatureHash(
        from_script_pubkey,
        mtx,
        0,
        SIGHASH_ALL,
        total_input,
        SigVersion::BASE
    );

    std::vector<unsigned char> sig;
    auto sign_f = key.Sign(sighash, sig);
    bcheck(sign_f);
    sig.push_back((unsigned char)SIGHASH_ALL);
    // scriptSig: <sig> <pubkey>
    mtx.vin[0].scriptSig = CScript() << sig << ToByteVector(pubkey);
    ///////////////////////////////////////////////////////////////////////////
    // Get the scriptPubKey for the input
    // CScript from_script_pubkey = GetScriptForDestination(from_dest);

    // Dummy previous tx output for signature
    // std::vector<CTxOut> vouts;
    // vouts.emplace_back(input_amount, from_script_pubkey);

    // Sign the input
    // TransactionSignatureCreator creator(&mtx, 0, input_amount, SIGHASH_ALL);
    // SignatureData sigdata;
    // bool sign_success = ProduceSignature(
    //     creator,
    //     key,
    //     from_script_pubkey,
    //     sigdata
    // );

    // bcheck(sign_success);
    // UpdateInput(mtx.vin[0], sigdata);
    ///////////////////////////////////////////////////////////////////////////

    // // -- SERIALIZE AND PRINT SIGNED TRANSACTION --
    // CDataStream ss_signed(SER_NETWORK, PROTOCOL_VERSION);
    // ss_signed << static_cast<CTransaction>(mtx);

     CTransaction txSigned(mtx);

    std::string signed_hex = EncodeHexTx(txSigned);
    std::cout << "Signed transaction: " << signed_hex << std::endl;
    std::cout << "Transaction size: " << GetVirtualTransactionSize(txSigned) << " vbytes" << std::endl;
    // std::cout << "Transaction size (raw bytes): " << GetSerializeSize(txSigned, PROTOCOL_VERSION) << " bytes" << std::endl;

    bcheck(signed_hex == "0200000001f7061814e7b778978ccb919355a97832c7336553d2bed7f39feca9\
d0150ab934010000006a473044022055c309fe3f6099f4f881d0fd960923eb91af\
f0d8ef3501a2fc04dce99aca609d0220174b9aec4fc22f6f81b637bbafec9554e4\
97ec2d9f3ca4992ee4209dd047443d012102898711e6bf63f5cbe1b38c05e89d6c\
391c59e9f8f695da44bf3d20ca674c8519ffffffff01d872000000000000160014\
c5b28d6bba91a2693a9b1876bcd3929323890fb200000000");

    std::cout << "end of chapter 2________________________________________________" << std::endl;
}

BOOST_AUTO_TEST_CASE(chapter_03)
{
    std::cout << "=== Chapter 3: P2SH Script Engineering - From Multi-signature to Time Locks ===" << std::endl;
    SelectParams(ChainType::MAIN);
    {
        CTxDestination dest;
        auto r = ExtractDestination(CScript() << OP_1 << ParseHex("2ceefa5fa770ff24f87c5475d76eab519eda6176b11dbe1618fcf755bfac5311"), dest);
        bcheck(r);
        std::cout << "script(OP_1 2ceefa5fa770ff24f87c5475d76eab519eda6176b11dbe1618fcf755bfac5311) to Address: " << EncodeDestination(dest) << ")" << std::endl;
        assert(EncodeDestination(dest) == "bc1p9nh05ha8wrljf7ru236awm4t2x0d5ctkkywmu9sclnm4t0av2vgs4k3au7");
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////
    {
        // The three compressed public keys (33 bytes each) provided by the user
        const std::string alice_hex = "02898711e6bf63f5cbe1b38c05e89d6c391c59e9f8f695da44bf3d20ca674c8519";
        const std::string bob_hex   = "0284b5951609b76619a1ce7f48977b4312ebe226987166ef044bfb374ceef63af5";
        const std::string carol_hex = "0317aa89b43f46a0c0cdbd9a302f2508337ba6a06d123854481b52de9c20996011";

        CPubKey alice_pk(ParseHex(alice_hex));
        CPubKey bob_pk(ParseHex(bob_hex));
        CPubKey carol_pk(ParseHex(carol_hex));

        brequire(alice_pk.IsValid() && alice_pk.IsCompressed());
        brequire(bob_pk.IsValid() && bob_pk.IsCompressed());
        brequire(carol_pk.IsValid() && carol_pk.IsCompressed());

        // Build the redeem script: 2 <pubkey1> <pubkey2> <pubkey3> 3 OP_CHECKMULTISIG
        CScript redeemScript;
        redeemScript << OP_2;
        redeemScript << ToByteVector(alice_pk);   // or bob_pk, carol_pk - order does not affect validity
        redeemScript << ToByteVector(bob_pk);
        redeemScript << ToByteVector(carol_pk);
        redeemScript << OP_3;
        redeemScript << OP_CHECKMULTISIG;

        // Optional: use the helper from Bitcoin Core (recommended for production code)
        // std::vector<CPubKey> pubkeys = {alice_pk, bob_pk, carol_pk};
        // CScript redeemScript = CreateMultisigRedeemscript(2, pubkeys);  // defined in src/script/standard.h

        std::string redeemHex = HexStr(redeemScript);

        std::cout << "=== 2-of-3 Multisig Redeem Script ===" << std::endl;
        std::cout << "Hex: " << redeemHex << std::endl;
        std::cout << "Size: " << redeemScript.size() << " bytes" << std::endl;

        // Verify it decodes correctly as multisig
        std::vector<std::vector<unsigned char>> extractedPubkeys;
        bcheck(TxoutType::MULTISIG == Solver(redeemScript, extractedPubkeys));  // from solver.h
        bequal(extractedPubkeys.size(), 5);
        bcheck(redeemHex == "522102898711e6bf63f5cbe1b38c05e89d6c391c59e9f8f695da44bf3d20ca674c8519210284b5951609b76619a1ce7f48977b4312ebe226987166ef044bfb374ceef63af5210317aa89b43f46a0c0cdbd9a302f2508337ba6a06d123854481b52de9c2099601153ae");
        std::cout << "Verified as 2-of-3 multisig (m=2, n=3)" << std::endl;
    }
    {
        // === Keys ===
        CKey key1, key2, key3;
        key1.MakeNewKey(true);
        key2.MakeNewKey(true);
        key3.MakeNewKey(true);

        CPubKey pub1 = key1.GetPubKey();
        CPubKey pub2 = key2.GetPubKey();
        CPubKey pub3 = key3.GetPubKey();

        // === redeemScript ===
        CScript redeemScript = CScript()
            << OP_2
            << ToByteVector(pub1)
            << ToByteVector(pub2)
            << ToByteVector(pub3)
            << OP_3
            << OP_CHECKMULTISIG;

        CScript scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));

        // === TX ===
        Txid txid = Txid::FromUint256(uint256("34b90a15d0a9ec9ff3d7bed2536533c73278a9559391cb8c9778b7e7141806f7"));
        CMutableTransaction tx;
        CAmount input_amount = 1600;
        tx.vin.emplace_back(COutPoint(txid, 1));
        tx.vout.emplace_back(1200, GetScriptForDestination(PKHash(pub1)));
            

        // === SigningProvider ===
        FillableSigningProvider keystore;

        // keystore.AddKey(key1);
        keystore.AddKey(key2);
        keystore.AddKey(key3);

        keystore.AddCScript(redeemScript);  //  REQUIRED for P2SH

        // === ProduceSignature ===
        SignatureData sigdata;

        bool ok = ProduceSignature(
            keystore,
            MutableTransactionSignatureCreator(tx, 0, input_amount, SIGHASH_ALL),
            scriptPubKey,
            sigdata
        );

        bcheck(ok);

        // === Apply result ===
        UpdateInput(tx.vin[0], sigdata);

        std::cout << "scriptSig: " << HexStr(tx.vin[0].scriptSig) << std::endl;

        // === Verify ===
        ScriptError err;
        bool verified = VerifyScript(
            tx.vin[0].scriptSig,
            scriptPubKey,
            nullptr,
            STANDARD_SCRIPT_VERIFY_FLAGS,
            MutableTransactionSignatureChecker(&tx, 0, input_amount, MissingDataBehavior::FAIL),
            &err
        );

        bcheck(verified);

        std::cout << "Final TX  " << __LINE__ << ":" << EncodeHexTx(CTransaction(tx)) << std::endl;
    }
    //a P2SH script that combines CSV time lock with P2PKH signature verification
    {
    // === 1. Key ===
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    CKeyID keyid = pubkey.GetID();

    // === 2. CSV lock ===
    const int csv_blocks = 5;

    // === 3. Build redeemScript ===
    CScript redeemScript = CScript()
        << csv_blocks
        << OP_CHECKSEQUENCEVERIFY
        << OP_DROP
        << OP_DUP
        << OP_HASH160
        << ToByteVector(keyid)
        << OP_EQUALVERIFY
        << OP_CHECKSIG;

    std::cout << "RedeemScript: " << HexStr(redeemScript) << std::endl;

    // === 4. P2SH scriptPubKey ===
    CScript scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));

    std::cout << "P2SH scriptPubKey: " << HexStr(scriptPubKey) << std::endl;

    // === 5. UTXO ===
    Txid txid = Txid::FromUint256(uint256("34b90a15d0a9ec9ff3d7bed2536533c73278a9559391cb8c9778b7e7141806f7"));

    // === 6. Build tx ===
    CMutableTransaction tx;
    tx.version = 2; // 🔴 REQUIRED for CSV
    // 🔴 IMPORTANT: sequence must enable CSV
    tx.vin.emplace_back(COutPoint(txid, 0), CScript{}, csv_blocks);  // sequence must enable CSV
    tx.vout.emplace_back(1500, GetScriptForDestination(PKHash(pubkey)));  
    
    // === 7. Create sighash ===
    CAmount input_amount = 2000;
    uint256 sighash = SignatureHash(
        redeemScript,
        tx,
        0,
        SIGHASH_ALL,
        input_amount,
        SigVersion::BASE
    );

    // === 8. Sign ===
    std::vector<unsigned char> sig;
    key.Sign(sighash, sig);
    sig.push_back((unsigned char)SIGHASH_ALL);

    // === 9. Build scriptSig ===
    tx.vin[0].scriptSig = CScript()
        << sig
        << ToByteVector(pubkey)
        << ToByteVector(redeemScript);

    std::cout << "scriptSig: " << HexStr(tx.vin[0].scriptSig) << std::endl;

    // === 10. Verify ===
    ScriptError err;
    bool ok = VerifyScript(
        tx.vin[0].scriptSig,
        scriptPubKey,
        nullptr,
        STANDARD_SCRIPT_VERIFY_FLAGS,
        MutableTransactionSignatureChecker(&tx, 0, input_amount, MissingDataBehavior::FAIL),
        &err
    );

    std::cout << "VerifyScript: " << ok << std::endl;
    bcheck(ok);

    // === 11. Final TX ===
    std::string tx_hex = EncodeHexTx(CTransaction(tx));

    std::cout << "Final TX: " << tx_hex << std::endl;
    std::cout << "vSize: " << GetVirtualTransactionSize(CTransaction(tx)) << " bytes" << std::endl;

    bcheck(!tx_hex.empty());        
    }
    {}
    std::cout << "end of chapter 3________________________________________________" << std::endl;
}

BOOST_AUTO_TEST_CASE(chapter_04)
{
        SelectParams(ChainType::TESTNET);

    std::cout << "=== Chapter 4: Building SegWit Transactions ===" << std::endl;
    //construct a p2wpkh transaction

        // === 1. Key ===
        CKey key;
        auto hex = ParseHex("e9873d79c6d87dc0fb6a5778633389dfa5c32fa27f99b5199abf2f9848ee0289");
        key.Set(hex.begin(), hex.end(), true);
        bcheck(key.IsValid());
        CPubKey pubkey = key.GetPubKey();
        CKeyID keyid = pubkey.GetID();
        
        // === 2. Build scriptPubKey ===
        auto to_address_str = "tb1qckeg66a6jx3xjw5mrpmte5ujjv3cjrajtvm9r4";
        bcheck(IsValidDestinationString(to_address_str));
        CTxDestination to_dest = DecodeDestination(to_address_str);
        CScript to_script_pubkey = GetScriptForDestination(to_dest);
        CScript scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(keyid));
        std::cout << "P2WPKH scriptPubKey: " << HexStr(to_script_pubkey) << std::endl;

        // === 3. UTXO ===
        Txid txid = Txid::FromUint256(uint256("1454438e6f417d710333fbab118058e2972127bdd790134ab74937fa9dddbc48"));

        // === 4. Build tx ===
        CMutableTransaction tx;
        tx.vin.emplace_back(COutPoint(txid, 0));
        tx.vout.emplace_back(666, to_script_pubkey);

            // === 5. Create sighash ===
        // P2WPKH uses P2PKH-style scriptCode
        CAmount input_amount = 1000;
        CScript scriptCode = CScript()
            << OP_DUP
            << OP_HASH160
            << ToByteVector(keyid)
            << OP_EQUALVERIFY
            << OP_CHECKSIG;

        // === 6. SegWit sighash ===
        uint256 sighash = SignatureHash(
            scriptCode,
            tx,
            0,
            SIGHASH_ALL,
            input_amount,
            SigVersion::WITNESS_V0
        );

        // === 6. Sign ===
        std::vector<unsigned char> sig;
        bool sign_f  = key.Sign(sighash, sig);
        brequire(sign_f);
        sig.push_back((unsigned char)SIGHASH_ALL);
        // === 7. Build scriptWitness ===
        tx.vin[0].scriptWitness.stack.push_back(sig);
        tx.vin[0].scriptWitness.stack.push_back(ToByteVector(pubkey));
        std::cout << "scriptWitness: " << tx.vin[0].scriptWitness.ToString() << std::endl;
        // === 8. Verify ===
        ScriptError err;
        bool ok = VerifyScript(
            CScript(),  // empty scriptSig for P2WPKH
            scriptPubKey,       
            &tx.vin[0].scriptWitness,  // witness
            STANDARD_SCRIPT_VERIFY_FLAGS,
            MutableTransactionSignatureChecker(&tx, 0, input_amount, MissingDataBehavior::ASSERT_FAIL  ),
            &err
        );
        std::cout << "VerifyScript: " << ok << ":" << ScriptErrorString(err) << std::endl;
        bcheck(ok);

        //print final tx hex
         CTransaction final_tx(tx);
         std::string tx_hex = EncodeHexTx(final_tx);
         std::cout << "Final TX: " << tx_hex << std::endl;
         std::cout << "vSize: " << GetVirtualTransactionSize(final_tx) << " bytes" << std::endl;
            bcheck(!tx_hex.empty());


    std::cout << "end of chapter 4________________________________________________" << std::endl;
}

// Given: internal_pubkey (secp256k1_xonly_pubkey)
// static uint256 bip341_taproot_tweak(const secp256k1_xonly_pubkey& internal_pub) {
//     // Serialize internal pubkey
//     unsigned char pk32[32];
//     secp256k1_xonly_pubkey_serialize(secp256k1_context_static, pk32, &internal_pub);

//     // 32 bytes of zero (no scripts)
//     unsigned char zeros[32] = {0};

//     // BIP341: SHA256(internal_pubkey_bytes || 32 zerobytes)
//     CSHA256 hasher;
//     hasher.Write(pk32, 32);
//     hasher.Write(zeros, 32);
//     uint256 tweak;
//     hasher.Finalize(tweak.begin());
//     return tweak;
// }

BOOST_AUTO_TEST_CASE(chapter_05)
{
    // Taproot key tweaking demonstration: K + tweak*G == (k + tweak)*G
    std::cout << "=== Chapter 5: Taproot: The Evolution of Bitcoin's Script System ===" << std::endl;
    // src/test/taproot_simple_tx_tests.cpp
    //sender="cPeon9fBsW2BxwJTALj3hGzh9vm8C52Uqsce7MzXGS1iFJkPF4AT",
    // receiver="tb1p53ncq9ytax924ps66z6al3wfhy6a29w8h6xfu27xem06t98zkmvsakd43h",
    // txid="b0f49d2f30f80678c6053af09f0611420aacf20105598330cb3f0ccb8ac7d7f0"

    // === 1. Generate internal key ===
    CKey key{DecodeSecret("cPeon9fBsW2BxwJTALj3hGzh9vm8C52Uqsce7MzXGS1iFJkPF4AT")}; brequire(key.IsValid());
    XOnlyPubKey internal_pubkey{key.GetPubKey()};

    // === 2. Taproot output key (no script tree, key-path only) ===
    auto ret = internal_pubkey.CreateTapTweak({}); brequire(ret);
    XOnlyPubKey output_pubkey = ret->first;
    brequire(!output_pubkey.IsNull());

    // ScriptPubKey: P2TR
    CScript scriptPubKey = GetScriptForDestination(DecodeDestination("tb1p53ncq9ytax924ps66z6al3wfhy6a29w8h6xfu27xem06t98zkmvsakd43h"));

    // === 3. Create funding UTXO ===
    // === 4. Build spending transaction ===
    CAmount amount = 100000; // 0.001 BTC
    CMutableTransaction mtx;
    mtx.vin.emplace_back(COutPoint(Txid::FromUint256(uint256("b0f49d2f30f80678c6053af09f0611420aacf20105598330cb3f0ccb8ac7d7f0")), 0));
    mtx.vout.emplace_back(amount - 1000, scriptPubKey);    
    // Output to recipient
    // send to anyone-can-spend (just for test)
    // mtx.vout[0].nValue = amount - 1000;
    // mtx.vout[0].scriptPubKey = CScript() << OP_TRUE;

    // === 5. Prepare signing data ===
    
    // Taproot keypair (tweaked private key)
    // KeyPair kp = key.ComputeKeyPair({});
    
    // Apply tweak: k + tweak mod n
    // kp = kp.TweakAdd(tweak);
    
    // === 6. Compute sighash ===
    // uint256 sighash = SignatureHashSchnorr(
    //     mtx,
    //     0,
    //     SIGHASH_DEFAULT,
    //     /*scriptPubKey=*/scriptPubKey,
    //     /*amount=*/amount,
    //     SigVersion::TAPROOT,
    //     txdata
    // );
    uint256 sighash;
    PrecomputedTransactionData txdata;
    txdata.Init(mtx, std::vector<CTxOut>{mtx.vout}, true);
    brequire(txdata.m_bip341_taproot_ready);

    FlatSigningProvider provider;
    provider.keys[key.GetPubKey().GetID()] = key;
    MutableTransactionSignatureCreator creator(mtx, 0, amount, &txdata, SIGHASH_SINGLE);
    std::vector<unsigned char> signature;
    uint256 merkle_root;
    BOOST_CHECK(creator.CreateSchnorrSig(provider, signature, internal_pubkey, nullptr, &merkle_root, SigVersion::TAPROOT));

    // === 8. Set witness ===
    mtx.vin[0].scriptWitness.stack = {signature};
    // === 9. Verify ===
    ScriptError err;
    bool ok = VerifyScript(
        CScript(), // empty scriptSig
        scriptPubKey,
        &mtx.vin[0].scriptWitness, // witness
        STANDARD_SCRIPT_VERIFY_FLAGS,
        MutableTransactionSignatureChecker(&mtx, 0, amount - 1000, txdata, MissingDataBehavior::ASSERT_FAIL),
        &err);
    std::cout << "VerifyScript: " << ok << ":" << ScriptErrorString(err) << std::endl;
    bcheck(ok);

    // === 9. Serialize and print ===
    CTransaction final_tx(mtx);
    std::string tx_hex = EncodeHexTx(final_tx);
    std::cout << "Taproot TX hex:\n" << tx_hex << std::endl;

    std::cout << "end of chapter 5________________________________________________" << std::endl;
}

BOOST_AUTO_TEST_CASE(chapter_05_SignatureHash)
{
        // === 1. Key ===
        CKey key{DecodeSecret("cPeon9fBsW2BxwJTALj3hGzh9vm8C52Uqsce7MzXGS1iFJkPF4AT")}; brequire(key.IsValid());
        bcheck(key.IsValid());
        CPubKey pubkey = key.GetPubKey();
        XOnlyPubKey xonly_pubkey{pubkey};
        CKeyID keyid = pubkey.GetID();
        
        // === 2. Build scriptPubKey ===
        auto to_address_str = "tb1qckeg66a6jx3xjw5mrpmte5ujjv3cjrajtvm9r4";
        bcheck(IsValidDestinationString(to_address_str));
        CTxDestination to_dest = DecodeDestination(to_address_str);
        CScript to_script_pubkey = GetScriptForDestination(to_dest);
        CScript scriptPubKey = GetScriptForDestination(WitnessV1Taproot(xonly_pubkey));
        std::cout << "P2WPKH scriptPubKey: " << HexStr(to_script_pubkey) << std::endl;

        // === 3. UTXO ===
        Txid txid = Txid::FromUint256(uint256("1454438e6f417d710333fbab118058e2972127bdd790134ab74937fa9dddbc48"));

        // === 4. Build tx ===
        CMutableTransaction tx;
        tx.vin.emplace_back(COutPoint(txid, 0));
        tx.vout.emplace_back(666, to_script_pubkey);

            // === 5. Create sighash ===
        // P2WPKH uses P2PKH-style scriptCode
        CAmount input_amount = 1000;
        CScript scriptCode = CScript()
            << OP_DUP
            << OP_HASH160
            << ToByteVector(keyid)
            << OP_EQUALVERIFY
            << OP_CHECKSIG;

        // === 6. SegWit sighash ===
        uint256 sighash = SignatureHash(
            scriptCode,
            tx,
            0,
            SIGHASH_ALL,
            input_amount,
            SigVersion::TAPROOT
        );

        // === 6. Sign ===
        std::vector<unsigned char> sig;
        brequire(key.Sign(sighash, sig));
        sig.push_back((unsigned char)SIGHASH_ALL);
        // === 7. Build scriptWitness ===
        tx.vin[0].scriptWitness.stack.push_back(sig);
        tx.vin[0].scriptWitness.stack.push_back(ToByteVector(pubkey));
        std::cout << "scriptWitness: " << tx.vin[0].scriptWitness.ToString() << std::endl;
        // === 8. Verify ===
        ScriptError err;
        bool ok = VerifyScript(
            CScript(),  // empty scriptSig for P2WPKH
            scriptPubKey,       
            &tx.vin[0].scriptWitness,  // witness
            STANDARD_SCRIPT_VERIFY_FLAGS,
            MutableTransactionSignatureChecker(&tx, 0, input_amount, MissingDataBehavior::ASSERT_FAIL  ),
            &err
        );
        std::cout << "VerifyScript: " << ok << ":" << ScriptErrorString(err) << std::endl;
        bcheck(ok);

        //print final tx hex
         CTransaction final_tx(tx);
         std::string tx_hex = EncodeHexTx(final_tx);
         std::cout << "Final TX: " << tx_hex << std::endl;
         std::cout << "vSize: " << GetVirtualTransactionSize(final_tx) << " bytes" << std::endl;
            bcheck(!tx_hex.empty());


    std::cout << "end of chapter 4________________________________________________" << std::endl;
}
BOOST_AUTO_TEST_CASE(chapter_05_gemini)
{
    // 1. 生成原始私钥
    CKey priv_key;
    priv_key.MakeNewKey(true);
    
    // 获取 X-only 内部公钥 (Internal Key)
    CPubKey pubkey = priv_key.GetPubKey();
    XOnlyPubKey internal_pubkey(pubkey);

    // 2. 计算 Taproot Output Key (Tweak)
    // 对于简单的 Key-path 支出，我们计算 Q = P + hash(P)G
    // ComputeTaprootOutputKey 返回转换后的公钥和 merkle_root（此处为空）
    auto ret = internal_pubkey.CreateTapTweak({}); brequire(ret);
    XOnlyPubKey output_pubkey = ret->first;
    brequire(!output_pubkey.IsNull());
    // 构造 P2TR scriptPubKey: OP_1 <output_pubkey>
    // CScript scriptPubKey = CScript() << OP_1 << ToByteVector(output_pubkey);
    CScript scriptPubKey = GetScriptForDestination(WitnessV1Taproot(output_pubkey));

    // 3. 构造交易
    CAmount amount = 100000; // 0.001 BTC
    CMutableTransaction mtx;
    // mtx.nVersion = 2;
//    mtx.vin.emplace_back(CTxIn(COutPoint(uint256S("01"), 0)));
//    mtx.vout.emplace_back(CTxOut(4999000, CScript() << OP_TRUE));
    mtx.vin.emplace_back(COutPoint(Txid::FromUint256(uint256("1454438e6f417d710333fbab118058e2972127bdd790134ab74937fa9dddbc48")), 0));
    mtx.vout.emplace_back(amount, scriptPubKey);    

    // 4. 准备签名哈希 (Sighash)
//    CAmount amount = 5000000;
    PrecomputedTransactionData txdata;
    txdata.Init(mtx, {CTxOut(amount, scriptPubKey)});
    
//    CHashWriter hasher = (SigVersion::TAPROOT, SIGHASH_DEFAULT);
    ScriptExecutionData execdata;
    uint256 sighash;
//    bool f = SignatureHashSchnorr(sighash, txdata, 0, SigVersion::TAPROOT, SIGHASH_DEFAULT);
    auto sighash_ret = SignatureHashSchnorr(sighash, execdata, mtx, 0, SIGHASH_DEFAULT, SigVersion::TAPROOT, txdata, MissingDataBehavior::FAIL);
    brequire(sighash_ret);

    // 5. 使用私钥生成 Schnorr 签名
    // 注意：必须对私钥进行同样的 Tweak 处理，否则无法通过验证
    // CKey tweaked_priv_key = priv_key;
    // uint256 tweak = internal_pubkey.ComputeTapTweakHash(std::nullopt);
    // BOOST_CHECK(tweaked_priv_key.TweakTaprootPrivKey(tweak));
    auto tweaked_priv_key = internal_pubkey.ComputeTapTweakHash({});

    std::vector<unsigned char> sig(64);
    // 这里调用的是 CKey 内部对 secp256k1_schnorsig_sign 的封装
    // BOOST_CHECK(tweaked_priv_key.SignSchnorr(sighash, sig, {}));

    // 6. 将签名放入 Witness 并验证
    // mtx.vin[0].scriptWitness.stack.push_back(sig);

    // ScriptError serror;
    // BOOST_CHECK(VerifyScript(CScript(), scriptPubKey, &mtx.vin[0].scriptWitness, 
    //                          STANDARD_SCRIPT_VERIFY_FLAGS, 
    //                          TransactionSignatureChecker(&mtx, 0, amount, txdata, MissingDataBehavior::FAIL), 
    //                          &serror));

    // BOOST_TEST_MESSAGE("Manual Signed Taproot Hex: " << EncodeHexTx(CTransaction(mtx)));
}
BOOST_AUTO_TEST_CASE(chapter_05_coliplot)
{
    // -- Key generation --
    CKey key; key.MakeNewKey(true);        // ECDSA secp256k1 key
    CPubKey pubkey = key.GetPubKey();

    // Taproot output: key path spend (tweaked pubkey, no script path)
    // Key Spend only (BIP 341 sec 5.3, no script path)
    XOnlyPubKey xonly{key.GetPubKey()};
    // key.GetPubKey().GetXOnlyPubKey(xonly);

    // Single-sig Taproot output: output key = xonly + tweak (with empty merkle root)
    uint256 merkle_root; // empty, scriptless Taproot
    // XOnlyPubKey taproot_out = xonly.AddTapTweak(merkle_root).first; // result + parity
    XOnlyPubKey taproot_out = xonly.CreateTapTweak(&merkle_root)->first; // result + parity
    CScript taproot_script = GetScriptForDestination(WitnessV1Taproot(taproot_out));

    // -- Create a funding transaction to give ourselves some coins --
    CMutableTransaction txfund;
    // txfund.nVersion = 2;
    txfund.vin.resize(1);
    txfund.vin[0].prevout = COutPoint(Txid::FromUint256(uint256("1454438e6f417d710333fbab118058e2972127bdd790134ab74937fa9dddbc48")), 0);
    txfund.vout.resize(1);
    txfund.vout[0].nValue = 10 * COIN;
    txfund.vout[0].scriptPubKey = taproot_script;

    // TxId txfund_hash = txfund.GetHash();

    // -- Build spending transaction --
    CMutableTransaction txspend;
    // txspend.nVersion = 2;
    txspend.vin.resize(1);
    txspend.vin[0].prevout = COutPoint(txfund.GetHash(), 0);
    txspend.vout.resize(1);
    txspend.vout[0].nValue = 9.9 * COIN;
    txspend.vout[0].scriptPubKey = GetScriptForDestination(PKHash(pubkey.GetID()));

    // -- Prepare input info --
    // Fake coin for input, mimics Coins view
    CCoinsViewCache coinsview(nullptr);
    CScript& scriptPubKeyOut = txfund.vout[0].scriptPubKey;
    coinsview.AddCoin(txfund.vin[0].prevout, Coin(txfund.vout[0], 1, false), true);

    // Construct signature for Taproot using Schnorr
    // - For bitcoin core's Taproot, use signers from src/script/sign.h

    // The raw sighash for key-path spends:
    int n_in = 0;
    uint8_t input_type = SIGHASH_DEFAULT;

    PrecomputedTransactionData txdata(txspend/*txfund*/);
    // uint256 sighash = SignatureHashSchnorr(scriptPubKeyOut, coinsview.AccessCoin(txspend.vin[0].prevout).out, txspend, n_in, input_type, &txdata);
    uint256 sighash;
    ScriptExecutionData execdata;
   auto sighash_ret = SignatureHashSchnorr(sighash, execdata, txspend, 0, SIGHASH_DEFAULT, SigVersion::TAPROOT, txdata, MissingDataBehavior::FAIL);
   brequire(sighash_ret);

//     // Sign with tweaked privkey
//     // key_tweaked = key + tweak
//     // Get corresponding tweaked key
//     KeyOriginInfo ignore;
//     uint256 tap_tweak;
//     TaprootBuilder builder;
//     builder.Finalize(nullptr, xonly, merkle_root, &tap_tweak);
//     CKey key_tweaked = key;
//     key_tweaked.Add(tap_tweak);

//     std::vector<unsigned char> sig64;
//     key_tweaked.SignSchnorr(sighash, sig64);

//     // -- Set witness --
//     txspend.wit.vtxinwit.resize(1);
//     txspend.wit.vtxinwit[0].scriptsig.clear();
//     txspend.wit.vtxinwit[0].scriptWitness.stack.push_back(sig64); // [sig]
//     // No control block, key path spend!

//     // -- Print transaction as hex --
//     CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
//     ssTx << static_cast<CTransaction>(txspend);
//     std::string hex = HexStr(ssTx);

//     std::cout << "Constructed Taproot transaction hex: " << hex << std::endl;

//     // -- Optionally: roundtrip decode and check --
//     CMutableTransaction tx2;
//     ssTx.Rewind();
//     ssTx >> tx2;

//     BOOST_CHECK_EQUAL(tx2.GetHash().ToString(), txspend.GetHash().ToString());
//     BOOST_CHECK_EQUAL(txspend.wit.vtxinwit[0].scriptWitness.stack.size(), 1);
//     BOOST_CHECK(sig64.size() == 64); // Schnorr sig

//     // -- Display full decoded TX for demo (not in real tests) --
//     std::cout << txspend.ToString() << std::endl;
}

BOOST_AUTO_TEST_CASE(chapter_05_grok)
{
    // This test constructs and signs a simple Taproot (P2TR) key-spend transaction.
    // It follows patterns similar to Bitcoin Core's transaction and script tests.
    // We use a fixed key for reproducibility (do not use in production).

    // Step 1: Create a private key and derive the internal pubkey (BIP 340 style)
    CKey key;
    key.MakeNewKey(true);  // compressed, but for Taproot we use x-only

    // For deterministic test, we can hardcode a test key (example from BIP 341 test vectors style)
    // Here we use a simple random key for illustration; in real tests often fixed.
    // To make fully deterministic, we can set a specific key:
    std::vector<unsigned char> privkey_bytes = ParseHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    key.Set(privkey_bytes.data(), privkey_bytes.data() + 32, true);

    const XOnlyPubKey internal_xonly = XOnlyPubKey(key.GetPubKey());

    // For a simple key-spend Taproot (no script path), the output key is the internal key tweaked with empty Merkle root.
    TaprootBuilder builder;  // empty tree for key-spend only
    // const TaprootSpendData spend_data = builder.Finalize(internal_xonly);
    // const XOnlyPubKey output_xonly = spend_data.GetOutputKey();  // tweaked key
    builder.Finalize(internal_xonly);
    // const XOnlyPubKey output_xonly = builder.GetOutput();  // tweaked key

    // Create the P2TR scriptPubKey: OP_1 <32-byte x-only pubkey>
    CScript spk = GetScriptForDestination(builder.GetOutput());

    BOOST_CHECK(spk.size() == 34);  // 1 (OP_1) + 1 (push 32) + 32 bytes

    // Step 2: Create a dummy funding transaction (the "previous" output we are spending)
    CMutableTransaction funding_tx;
    // funding_tx.nVersion = 2;
    funding_tx.vin.resize(1);
    funding_tx.vin[0].prevout = COutPoint(Txid::FromUint256(uint256("1454438e6f417d710333fbab118058e2972127bdd790134ab74937fa9dddbc48")), 0);
    funding_tx.vout.resize(1);
    funding_tx.vout[0].nValue = 100000000;  // 1 BTC in satoshis for simplicity
    funding_tx.vout[0].scriptPubKey = spk;

    // Step 3: Construct the spending transaction (simple 1-input, 1-output key spend)
    CMutableTransaction spend_tx;
    // spend_tx.nVersion = 2;
    spend_tx.vin.resize(1);
    spend_tx.vin[0].prevout = COutPoint(funding_tx.GetHash(), 0);
    spend_tx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;

    // Output: send almost everything back to a new P2TR address (or anyone-can-spend for test, but let's use another P2TR)
    // For simplicity, use a dummy output script (e.g., another P2TR or OP_RETURN, but to keep valid: another key-spend P2TR)
    CKey dest_key;
    dest_key.MakeNewKey(true);
    XOnlyPubKey dest_xonly(dest_key.GetPubKey());
    CScript dest_spk = GetScriptForDestination(WitnessV1Taproot(dest_xonly));

    spend_tx.vout.resize(1);
    spend_tx.vout[0].nValue = 99990000;  // 0.9999 BTC, fee = 10000 sat
    spend_tx.vout[0].scriptPubKey = dest_spk;

    // Step 4: Prepare for signing (key spend path)
    // We need a signing provider with the private key
    FlatSigningProvider provider;
    provider.keys[key.GetPubKey().GetID()] = key;  // map by pubkey ID

    // For Taproot key spend, we use the tweaked private key internally via TaprootSigner or manual
    // Bitcoin Core uses PSBT or direct signing helpers in recent versions.
    // Here we manually compute the sighash and sign (similar to how wallet does it).

    // Create prevouts for SIGHASH_ALL (required for Taproot key spend in most cases)
    std::vector<CTxOut> prevouts;
    prevouts.push_back(funding_tx.vout[0]);

    // Compute Taproot sighash (BIP 341)
    uint256 sighash;
    PrecomputedTransactionData txdata(spend_tx);
    // txdata.Init(spend_tx, std::move(prevouts));  // for efficiency

    // // SIGHASH_DEFAULT (0x00) is common for Taproot key spend
    // sighash = SignatureHash(spend_tx, 0, SIGHASH_DEFAULT, prevouts[0].nValue, prevouts[0].scriptPubKey, txdata);

    // BOOST_CHECK(!sighash.IsNull());

    // // Sign with Schnorr (BIP 340)
    // // For key spend, we use the tweaked private key (tweak = tagged_hash("TapTweak", internal_key || merkle_root))
    // // Since merkle root is empty here, we can use:
    // const uint256 tweak = ComputeTapTweak(internal_xonly, uint256{});  // empty root
    // CKey tweaked_key = key;
    // bool tweaked = tweaked_key.TweakAdd(tweak);  // in practice use secp context
    // BOOST_CHECK(tweaked);

    // // In real code we use secp256k1_schnorr_sign
    // // Here we simulate signing (Bitcoin Core uses secp256k1 via CKey::SignSchnorr or similar helpers)
    // std::vector<unsigned char> sig(64);
    // // Note: In actual Bitcoin Core unit tests, they often use the full interpreter or PSBT.
    // // For this example, we assume signing succeeds and construct a valid witness.

    // // For a complete working example in unit test style, we can use the wallet's signing or manual witness.
    // // Simpler approach used in many tests: construct witness with a dummy signature and check structure.

    // // To make it "sign" properly, let's use a placeholder but valid-looking witness for key-spend.
    // // Real signing requires the secp context properly set up (see src/test/util/setup_common.cpp).

    // // Better: Use MutableTransactionSignatureCreator or PSBT for real signing in test.

    // // For this demonstration, we build the witness manually with a 64-byte Schnorr sig + SIGHASH_DEFAULT (0x00)
    // std::vector<unsigned char> schnorr_sig(64, 0x01);  // placeholder bytes (in real test: actual signature)
    // schnorr_sig[0] = 0x00;  // example r value start

    // CScriptWitness witness;
    // witness.stack.push_back(schnorr_sig);  // 64-byte signature for key spend (no sighash byte if DEFAULT, but often appended as 0x00)

    // // For SIGHASH_DEFAULT, the witness is just the 64-byte signature.
    // spend_tx.vin[0].scriptWitness = witness;

    // // Step 5: Print the transaction (hex)
    // const std::string tx_hex = EncodeHexTx(CTransaction(spend_tx));
    // std::cout << "Constructed and 'signed' Simple Taproot Key-Spend Transaction:" << std::endl;
    // std::cout << tx_hex << std::endl;

    // // Basic checks
    // BOOST_CHECK(spend_tx.vin.size() == 1);
    // BOOST_CHECK(spend_tx.vout.size() == 1);
    // BOOST_CHECK(spend_tx.vin[0].scriptWitness.stack.size() == 1);
    // BOOST_CHECK(spend_tx.vin[0].scriptWitness.stack[0].size() == 64);  // Schnorr sig for key spend

    // // In a real full test, you would verify with:
    // // ScriptError serror;
    // // BOOST_CHECK(VerifyScript(CScript(), prevouts[0].scriptPubKey, &spend_tx.vin[0].scriptWitness, ... ));
    // // But that requires full context and a valid signature.
}

BOOST_AUTO_TEST_SUITE_END()
