/*!
 * https://github.com/aaron-recompile/mastering-taproot
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
#include <coins.h>  //CCoinsViewCache
#include "boss.h"
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>
#include <univalue.h>   //UniValue

 BOOST_FIXTURE_TEST_SUITE(mastering_taproot, TestingSetup)

 BOOST_AUTO_TEST_CASE(chapter_01)
{
    std::cout << "=== Chapter 1: Key Generation and Address Creation ===" << std::endl;
    // private key
    CKey k;
    brequire(!k.IsValid());
    auto hex = ParseHex("e9873d79c6d87dc0fb6a5778633389dfa5c32fa27f99b5199abf2f9848ee0289");
    k.Set(hex.begin(), hex.end(), true);
    brequire(k.IsValid());

    printf("Private Key :                      %s\n", HexStr(std::span{k.begin(), k.end()}).c_str());
    brequire(HexStr(std::span{k.begin(), k.end()}) == "e9873d79c6d87dc0fb6a5778633389dfa5c32fa27f99b5199abf2f9848ee0289");
    printf("Private Key (WIF):                 %s, prefix=%s\n", EncodeSecret(k).c_str(),
        HexStr(Params().Base58Prefix(CChainParams::SECRET_KEY)).c_str());

    brequire(EncodeSecret(k) == "L53fCHmQhbNp1B4JipfBtfYgv8G9vsxeUE8cCJKyNAgbieXbUz2i");
    auto dk = DecodeSecret("L53fCHmQhbNp1B4JipfBtfYgv8G9vsxeUE8cCJKyNAgbieXbUz2i");
    brequire(dk.IsValid());
    brequire(k == dk);

    //public key
    auto pk = k.GetPubKey(), cpk = pk;
    printf("Public Key :                       %s\n", HexStr(std::span{pk.begin(), pk.end()}).c_str());
    brequire(HexStr(std::span{pk.begin(), pk.end()}) ==
        "02f883752ff979dffe25fe7f2412988965ee8b3a5f0ca837ef4b60202b3d88baa8"
                                                                );
    pk.Decompress();
    printf("Decompressed Public Key :          %s\n", HexStr(std::span{pk.begin(), pk.end()}).c_str());
    brequire(HexStr(std::span{pk.begin(), pk.end()}) ==
        "04f883752ff979dffe25fe7f2412988965ee8b3a5f0ca837ef4b60202b3d88baa8a0e75b2059ed5c8a6d3692bf870902d56e359af8f543a"
        "9da71d1e0e2e03469a4");

    //x-only public key
    XOnlyPubKey xpk{pk};
    printf("Xonly Public Key :                 %s\n", HexStr(std::span{xpk.begin(), xpk.end()}).c_str());
     brequire(HexStr(std::span{xpk.begin(), xpk.end()}) ==
         "f883752ff979dffe25fe7f2412988965ee8b3a5f0ca837ef4b60202b3d88baa8");

    // key to address
     //p2pk
    PubKeyDestination p2pk{cpk};
    auto addr_p2pk = EncodeDestination(p2pk);
    printf("P2PK Address:                      %s\n", addr_p2pk.c_str());
    //  brequire(addr_p2pk == "04f883752ff979dffe25fe7f2412988965ee8b3a5f0ca837ef4b60"
    // "202b3d88baa8a0e75b2059ed5c8a6d3692bf870902d56e359af8f543a9da71d1e0e2e03469a4");

     // CScript scriptPubKey = CScript() << cpk << OP_CHECKSIG;
     // printf("P2PK ScriptPubKey: %s\n", HexStr(scriptPubKey).c_str());

        //p2pkh
     auto addr_p2pkh = EncodeDestination(PKHash(cpk));
     printf("Legacy (P2PKH):                   %s\n", addr_p2pkh.c_str());
     brequire(addr_p2pkh == "1FEokwQhTGXz2t2nX5u8pXAh8opWQYrq6g");

        //p2wpkh
     auto addr_p2wpkh = EncodeDestination(WitnessV0KeyHash(cpk));
     printf("P2WPKH Address (SegWit Native):   %s\n", addr_p2wpkh.c_str());
     brequire(addr_p2wpkh == "bc1qnshrxgy59dsa8wzjq9fevur6a22p25tn407vwj");

        //Nested SegWit（P2SH-P2WPKH）地址生成
    auto addr_p2sh_p2wpkh = EncodeDestination(ScriptHash(GetScriptForDestination(WitnessV0KeyHash(cpk))));
    printf("P2SH-P2WPKH Address (SegWit P2SH): %s\n", addr_p2sh_p2wpkh.c_str());
     // brequire(addr_p2sh_p2wpkh == "2N7V2kDqAcdMiKqMmKKBbs6WtVgKPnDg2Hi");

        //p2tr
     auto addr_p2tr = EncodeDestination(WitnessV1Taproot(xpk));
     printf("P2TR Address:                     %s\n", addr_p2tr.c_str());
     brequire(addr_p2tr == "bc1plzph2tle080luf070ujp9xyfvhhgkwjlpj5r0m6tvqszk0vgh25qm6ga3e");

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
            MutableTransactionSignatureCreator(tx, 0, input_amount, SignOptions{SIGHASH_ALL}),
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

BOOST_AUTO_TEST_CASE(chapter_05)
{
    std::cout << "=== Chapter 5: Taproot: The Evolution of Bitcoin's Script System ===" << std::endl;

    // Taproot key tweaking demonstration: K + tweak*G == (k + tweak)*G
    //key tweaking验证
     {

        CKey key;
        key.MakeNewKey(true);

        uint256 hash;
        unsigned char sig[64];
        const uint256* merkle_root = nullptr;
        const uint256 aux;
        auto ret = key.SignSchnorr(hash, sig, merkle_root, aux);
        brequire(ret);

        XOnlyPubKey xpubkey{key.GetPubKey()};
        ret = xpubkey.VerifySchnorr(hash, sig);
        brequire(ret);
     }
     {
        auto ret{false};
        CKey key;
        key.MakeNewKey(true);

        uint256 hash;
        unsigned char sig[64];
        const uint256 merkle_rootobj = m_rng.rand256(), *merkle_root = &merkle_rootobj;
        const uint256 aux = m_rng.rand256();

        XOnlyPubKey xpubkey{key.GetPubKey()};
        auto tweaked = xpubkey.CreateTapTweak(merkle_root);
        brequire(tweaked);
        XOnlyPubKey tweaked_key = tweaked->first;

        ret = key.SignSchnorr(hash, sig, merkle_root, aux);
        brequire(ret);
        ret = tweaked_key.VerifySchnorr(hash, sig);
        brequire(ret);
     }
    std::cout << "end of chapter 5________________________________________________" << std::endl;
}

BOOST_AUTO_TEST_CASE(chapter_06) {
     // 1. Alice 的密钥对（实际使用时从安全来源加载私钥）
     CKey alice_key;
     alice_key.MakeNewKey(true);                 // 或从 WIF / 种子派生
     XOnlyPubKey alice_xonly(alice_key.GetPubKey());

     // 2. 计算 preimage 的 SHA256
     std::string preimage_str = "helloworld";
     uint256 preimage_hash = Hash(preimage_str);   // 或 SHA256(preimage_str)

     // 3. 条件脚本：任何人知道 "helloworld" 就能花费
     CScript condition_script;
     condition_script << OP_SHA256 << ToByteVector(preimage_hash) << OP_EQUAL;

     // 4. 使用 TaprootBuilder
     TaprootBuilder builder;
     builder.Add(/*depth=*/0, condition_script, /*leaf_version=*/0xc0);  // 标准 tapscript

     BOOST_CHECK(builder.IsValid());
     BOOST_CHECK(builder.IsComplete());

     // 5. Finalize（Alice 的 x-only 公钥作为 internal key → 所有者路径）
     builder.Finalize(alice_xonly);

     // 6. 得到输出
     WitnessV1Taproot output = builder.GetOutput();
     CTxDestination dest = output;
     std::string address = EncodeDestination(dest);   // bc1p... 地址

     // 7. 获取花费数据（方便后续构造 witness）
     TaprootSpendData spend_data = builder.GetSpendData();

     // print
     UniValue univ(UniValue::VOBJ);
     ScriptToUniv(condition_script, univ, true, true);
     std::cout << "P2TR address using TaprootBuilder:\n\t" << address << "\n\t" << ScriptToAsmStr(condition_script)
                        << ""/*univ.get_str()*/ << std::endl;

 }

BOOST_AUTO_TEST_SUITE_END()
