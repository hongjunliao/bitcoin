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

struct TestnetSetup : public TestingSetup {
	TestnetSetup()
        : TestingSetup{ChainType::TESTNET} {}
};

 BOOST_FIXTURE_TEST_SUITE(mastering_taproot, TestnetSetup)

 BOOST_AUTO_TEST_CASE(chpater_01)
{
    std::cout << "=== Chapter 1: Key Generation and Address Creation ===" << std::endl;
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

     std::cout << "end of chapter 1________________________________________________" << std::endl;
}

BOOST_AUTO_TEST_CASE(chpater_02)
{
    std::cout << "=== Chapter 2:Bitcoin Script and P2PKH Fundamentals ===" << std::endl;
    // -- SETUP --
    // Testnet WIF private key
    std::string wif_str = "cPeon9fBsW2BxwJTALj3hGzh9vm8C52Uqsce7MzXGS1iFJkPF4AT";
    // CKey key;
    // key.Set(key.DecodeBase58Check(wif_str), wif_str[0] == 'c' || wif_str[0] == '9'); // testnet/compressed
    CKey key = DecodeSecret(wif_str);
    BOOST_CHECK(key.IsValid());

    CPubKey pubkey = key.GetPubKey();

    // // Sender address: myYHJtG3cyoRseuTwvViGHgP2efAvZkYa4
    auto from_address_str = "myYHJtG3cyoRseuTwvViGHgP2efAvZkYa4";
    BOOST_CHECK(IsValidDestinationString(from_address_str));
    CTxDestination from_dest = DecodeDestination(from_address_str);

    // // Receiver SegWit address: tb1qckeg66a6jx3xjw5mrpmte5ujjv3cjrajtvm9r4
    auto to_address_str = "tb1qckeg66a6jx3xjw5mrpmte5ujjv3cjrajtvm9r4";
    BOOST_CHECK(IsValidDestinationString(to_address_str));
    CTxDestination to_dest = DecodeDestination(to_address_str);

    BOOST_CHECK(IsValidDestination(from_dest));
    BOOST_CHECK(IsValidDestination(to_dest));

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
    BOOST_CHECK(sign_f);
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

    // BOOST_CHECK(sign_success);
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

    BOOST_CHECK(signed_hex == "0200000001f7061814e7b778978ccb919355a97832c7336553d2bed7f39feca9\
d0150ab934010000006a473044022055c309fe3f6099f4f881d0fd960923eb91af\
f0d8ef3501a2fc04dce99aca609d0220174b9aec4fc22f6f81b637bbafec9554e4\
97ec2d9f3ca4992ee4209dd047443d012102898711e6bf63f5cbe1b38c05e89d6c\
391c59e9f8f695da44bf3d20ca674c8519ffffffff01d872000000000000160014\
c5b28d6bba91a2693a9b1876bcd3929323890fb200000000");

    std::cout << "end of chapter 2________________________________________________" << std::endl;
}

BOOST_AUTO_TEST_CASE(chpater_03)
{
    std::cout << "=== Chapter 3: P2SH Script Engineering - From Multi-signature to Time Locks ===" << std::endl;
    SelectParams(ChainType::MAIN);
    {
        CTxDestination dest;
        auto r = ExtractDestination(CScript() << OP_1 << ParseHex("2ceefa5fa770ff24f87c5475d76eab519eda6176b11dbe1618fcf755bfac5311"), dest);
        BOOST_CHECK(r);
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

        BOOST_REQUIRE(alice_pk.IsValid() && alice_pk.IsCompressed());
        BOOST_REQUIRE(bob_pk.IsValid() && bob_pk.IsCompressed());
        BOOST_REQUIRE(carol_pk.IsValid() && carol_pk.IsCompressed());

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
        BOOST_CHECK(TxoutType::MULTISIG == Solver(redeemScript, extractedPubkeys));  // from solver.h
        BOOST_CHECK_EQUAL(extractedPubkeys.size(), 5);
        BOOST_CHECK(redeemHex == "522102898711e6bf63f5cbe1b38c05e89d6c391c59e9f8f695da44bf3d20ca674c8519210284b5951609b76619a1ce7f48977b4312ebe226987166ef044bfb374ceef63af5210317aa89b43f46a0c0cdbd9a302f2508337ba6a06d123854481b52de9c2099601153ae");
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

        BOOST_CHECK(ok);

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

        BOOST_CHECK(verified);

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
    BOOST_CHECK(ok);

    // === 11. Final TX ===
    std::string tx_hex = EncodeHexTx(CTransaction(tx));

    std::cout << "Final TX: " << tx_hex << std::endl;
    std::cout << "vSize: " << GetVirtualTransactionSize(CTransaction(tx)) << " bytes" << std::endl;

    BOOST_CHECK(!tx_hex.empty());        
    }
    {}
    std::cout << "end of chapter 3________________________________________________" << std::endl;
}

BOOST_AUTO_TEST_CASE(chpater_04)
{
        SelectParams(ChainType::TESTNET);

    std::cout << "=== Chapter 4: Building SegWit Transactions ===" << std::endl;
    //construct a p2wpkh transaction

        // === 1. Key ===
        CKey key;
        auto hex = ParseHex("e9873d79c6d87dc0fb6a5778633389dfa5c32fa27f99b5199abf2f9848ee0289");
        key.Set(hex.begin(), hex.end(), true);
        BOOST_CHECK(key.IsValid());
        CPubKey pubkey = key.GetPubKey();
        CKeyID keyid = pubkey.GetID();
        
        // === 2. Build scriptPubKey ===
        auto to_address_str = "tb1qckeg66a6jx3xjw5mrpmte5ujjv3cjrajtvm9r4";
        BOOST_CHECK(IsValidDestinationString(to_address_str));
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
        key.Sign(sighash, sig);
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
        BOOST_CHECK(ok);

        //print final tx hex
         CTransaction final_tx(tx);
         std::string tx_hex = EncodeHexTx(final_tx);
         std::cout << "Final TX: " << tx_hex << std::endl;
         std::cout << "vSize: " << GetVirtualTransactionSize(final_tx) << " bytes" << std::endl;
            BOOST_CHECK(!tx_hex.empty());


    std::cout << "end of chapter 4________________________________________________" << std::endl;
}
BOOST_AUTO_TEST_SUITE_END()
