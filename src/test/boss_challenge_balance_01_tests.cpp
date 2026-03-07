#include <boost/test/unit_test.hpp>

// Bitcoin Core headers
#include <key.h>
#include <pubkey.h>
#include <chainparams.h>
#include <base58.h>
#include <bech32.h>
#include <crypto/hmac_sha512.h>
#include <crypto/sha256.h>
// #include <script/standard.h>
#include <script/descriptor.h>
#include <util/strencodings.h>
// #include <util/system.h>
#include <uint256.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <cassert>

#include <key_io.h>
#include <secp256k1.h>
#include <univalue.h>
/**
 * ============================================================================
 * Bitcoin Core Taproot Descriptor 余额查询完整测试
 * ============================================================================
 * 
 * 测试流程：
 * 1. 解析Descriptor字符串 (tr(tprv.../path)#checksum)
 * 2. 解码BIP32扩展私钥 (Base58Check)
 * 3. 使用HMAC-SHA512进行BIP32子密钥派生 (m/86h/1h/0h/0/i)
 * 4. 对每个派生的私钥生成Schnorr公钥 (XOnlyPubKey)
 * 5. 构造Taproot脚本和地址 (bc1p...)
 * 6. 调用bitcoin-cli RPC scantxoutset扫描UTXO
 * 7. 解析并输出最终余额
 */

BOOST_AUTO_TEST_SUITE(taproot_descriptor_balance_complete_tests)

// ============================================================================
// 辅助类和函数
// ============================================================================

/**
 * BIP32链码，用于派生
 * 格式：32字节
 */
using ChainCode = std::array<unsigned char, 32>;

/**
 * BIP32扩展私钥结构
 * 用于存储解析后的扩展密钥信息
 */
struct ExtendedPrivateKey {
    unsigned char nDepth;                    // 深度
    unsigned char vchFingerprint[4];         // 父密钥指纹
    unsigned int nChild;                     // 子索引
    ChainCode chaincode;                     // 链码（32字节）
    CKey key;                                // 私钥
    
    bool IsValid() const {
        return key.IsValid();
    }
};

/**
 * BIP32派生参数
 * 用于表示派生路径中的一个索引
 */
struct DerivationIndex {
    uint32_t index;
    bool hardened;
    
    uint32_t GetValue() const {
        return hardened ? (index | 0x80000000) : index;
    }
    
    std::string ToString() const {
        std::string result = std::to_string(index);
        if (hardened) result += "h";
        return result;
    }
};

// ============================================================================
// Step 1: 解析Descriptor
// ============================================================================

/**
 * 解析Descriptor字符串
 * 格式: tr(tprv.../path)#checksum
 * 
 * 返回: (extended_key_str, derivation_path_str, checksum)
 */
std::tuple<std::string, std::string, std::string> ParseDescriptorString(
    const std::string& descriptor)
{
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    std::cout << "Step 1: 解析Descriptor字符串" << std::endl;
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    
    std::cout << "Descriptor: " << descriptor << std::endl << std::endl;
    
    // 查找 tr( 开始
    size_t tr_start = descriptor.find("tr(");
    if (tr_start == std::string::npos) {
        throw std::runtime_error("Invalid descriptor: 'tr(' not found");
    }
    
    // 查找tprv开始
    size_t tprv_start = descriptor.find("tprv", tr_start);
    if (tprv_start == std::string::npos) {
        throw std::runtime_error("Invalid descriptor: 'tprv' not found");
    }
    
    // 查找路径分隔符 /
    size_t path_start = descriptor.find("/", tprv_start);
    if (path_start == std::string::npos) {
        throw std::runtime_error("Invalid descriptor: derivation path '/' not found");
    }
    
    // 提取extended key (tprv...直到/)
    std::string extended_key = descriptor.substr(tprv_start, path_start - tprv_start);
    
    // 查找路径结束 ) 或 *
    size_t path_end = descriptor.find(")", path_start);
    if (path_end == std::string::npos) {
        throw std::runtime_error("Invalid descriptor: ')' not found");
    }
    
    // 提取派生路径
    std::string derivation_path = descriptor.substr(path_start, path_end - path_start);
    
    // 提取checksum #xxx
    size_t checksum_start = descriptor.find("#", path_end);
    std::string checksum;
    if (checksum_start != std::string::npos) {
        checksum = descriptor.substr(checksum_start + 1);
    }
    
    std::cout << "✓ Extended Private Key: " << extended_key << std::endl;
    std::cout << "✓ Derivation Path: " << derivation_path << std::endl;
    std::cout << "✓ Checksum: " << (checksum.empty() ? "(none)" : checksum) << std::endl;
    std::cout << std::endl;
    
    return std::make_tuple(extended_key, derivation_path, checksum);
}

// ============================================================================
// Step 2: 解码BIP32扩展私钥
// ============================================================================

/**
 * 解码Base58Check编码的扩展私钥
 * 
 * 结构 (78��节):
 * [0:4]    版本号 (4字节)
 * [4:5]    深度 (1字节)
 * [5:9]    父密钥指纹 (4字节)
 * [9:13]   子索引 (4字节，大端序)
 * [13:45]  链码 (32字节)
 * [45:46]  0x00前缀 (1字节，用于私钥)
 * [46:78]  私钥 (32字节)
 * 
 * 总计: 78字节
 */
ExtendedPrivateKey DecodeExtendedKey(const std::string& extended_key_str)
{
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    std::cout << "Step 2: 解码BIP32扩展私钥" << std::endl;
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    
    // 从Base58Check解码
    std::vector<unsigned char> decoded;
    if (!DecodeBase58Check(extended_key_str, decoded, 78)) {
        throw std::runtime_error("Failed to Base58Check decode extended key");
    }
    
    // 验证长度
    const size_t EXTKEY_SIZE = 78;
    if (decoded.size() != EXTKEY_SIZE) {
        throw std::runtime_error(strprintf(
            "Invalid extended key size: %d (expected %d)", 
            decoded.size(), EXTKEY_SIZE));
    }
    
    std::cout << "✓ Base58Check解码成功 (78字节)" << std::endl;
    std::cout << std::endl;
    
    // 解析字段
    std::cout << "解析字段:" << std::endl;
    
    // [0:4] 版本号
    unsigned int version = ReadBE32(decoded.data());
    std::cout << "  版本号: 0x" << HexStr(std::span{decoded.data(), 4}) << std::endl;
    
    // [4:5] 深度
    unsigned char depth = decoded[4];
    std::cout << "  深度: " << (int)depth << std::endl;
    
    // [5:9] 父指纹
    unsigned char parent_fp[4];
    std::copy(decoded.begin() + 5, decoded.begin() + 9, parent_fp);
    std::cout << "  父指纹: " << HexStr(std::span{parent_fp, 4}) << std::endl;
    
    // [9:13] 子索引
    unsigned int child_index = ReadBE32(decoded.data() + 9);
    std::cout << "  子索引: " << child_index << std::endl;
    
    // [13:45] 链码
    ChainCode chaincode;
    std::copy(decoded.begin() + 13, decoded.begin() + 45, chaincode.begin());
    std::cout << "  链码: " << HexStr(chaincode) << std::endl;
    
    // [45:46] 0x00前缀
    unsigned char prefix = decoded[45];
    if (prefix != 0x00) {
        throw std::runtime_error("Invalid private key prefix (expected 0x00)");
    }
    std::cout << "  前缀: 0x00 ✓" << std::endl;
    
    // [46:78] 私钥
    std::vector<unsigned char> privkey_bytes(decoded.begin() + 46, decoded.end());
    std::cout << "  私钥: " << HexStr(privkey_bytes) << std::endl;
    std::cout << std::endl;
    
    // 构造结果
    ExtendedPrivateKey result;
    result.nDepth = depth;
    std::copy(parent_fp, parent_fp + 4, result.vchFingerprint);
    result.nChild = child_index;
    result.chaincode = chaincode;
    
    // 从私钥字节构造CKey
    result.key.Set(privkey_bytes.begin(), privkey_bytes.end(), true);
    if (!true) {
        throw std::runtime_error("Failed to construct CKey from private key bytes");
    }
    
    if (!result.key.IsValid()) {
        throw std::runtime_error("Invalid private key");
    }

    std::cout << "✓ 扩展私钥解码成功，私钥有效" << std::endl;
    std::cout << std::endl;
    
    return result;
}

// ============================================================================
// Step 3: BIP32密钥派生 (HMAC-SHA512)
// ============================================================================

/**
 * BIP32哈希函数 (内部辅助)
 * 
 * 使用HMAC-SHA512计算派生:
 * I = HMAC-SHA512(Key = cpar, Data = X || nChild)
 * 
 * 其中:
 * - cpar: 父链码
 * - X: 密钥数据 (公钥或私钥)
 * - nChild: 子索引 (4字节，大端序)
 * - I: 64字节结果
 *   - I_L (前32字节): 用于tweaking
 *   - I_R (后32字节): 新链码
 */
void BIP32Hash(
    const ChainCode& parent_chaincode,
    uint32_t nChild,
    unsigned char header,
    const unsigned char* data,
    unsigned char* out)
{
    unsigned char num[4];
    WriteBE32(num, nChild);
    
    CHMAC_SHA512 hmac(parent_chaincode.data(), parent_chaincode.size());
    hmac.Write(&header, 1);
    hmac.Write(data, 32);
    hmac.Write(num, 4);
    hmac.Finalize(out);
}

/**
 * 派生单个子密钥
 * 
 * 标准派生 (non-hardened, nChild < 0x80000000):
 * I = HMAC-SHA512(cpar, ser_P(point(k_par)) || ser_32(i))
 * 
 * 强化派生 (hardened, nChild >= 0x80000000):
 * I = HMAC-SHA512(cpar, 0x00 || ser_256(k_par) || ser_32(i))
 */
ExtendedPrivateKey DeriveChildKey(
    const ExtendedPrivateKey& parent,
    uint32_t nChild)
{
    ExtendedPrivateKey child;
    child.nDepth = parent.nDepth + 1;
    
    // 设置父指纹
    BOOST_REQUIRE(parent.IsValid());
    auto pk = parent.key.GetPubKey();
    std::vector<unsigned char> parent_pubkey_bytes{pk.begin(), pk.end()};
    uint160 parent_pubkey_id = Hash160(parent_pubkey_bytes);
    std::copy(parent_pubkey_id.begin(), parent_pubkey_id.begin() + 4, child.vchFingerprint); 
    child.nChild = nChild;
    
    // 64字节的派生结果
    unsigned char vout[64];
    
    // 判断是否为强化派生
    bool is_hardened = (nChild >> 31) != 0;
    
    if (!is_hardened) {
        // 标准派生：使用公钥
        CPubKey pubkey = parent.key.GetPubKey();
        assert(pubkey.size() == CPubKey::COMPRESSED_SIZE);
        // 格式: HMAC-SHA512(cpar, 公钥 || nChild)
        BIP32Hash(parent.chaincode, nChild, *pubkey.begin(), pubkey.begin() + 1, vout);
    } else {
        // 强化派生：使用私钥
        // 格式: HMAC-SHA512(cpar, 0x00 || 私钥 || nChild)
        assert(parent.key.size() == 32);
        unsigned char data[33];
        data[0] = 0x00;
        std::copy(parent.key.begin(), parent.key.begin() + 32, (std::byte*)data + 1);
        
        unsigned char num[4];
        WriteBE32(num, nChild);
        CHMAC_SHA512 hmac(parent.chaincode.data(), parent.chaincode.size());
        hmac.Write(data, 33);
        hmac.Write(num, 4);
        hmac.Finalize(vout);
    }
    
    // vout的后32字节是新的链码
    std::copy(vout + 32, vout + 64, child.chaincode.begin());
    
    // vout的前32字节 (tweak) 用于派生子私钥
    // 子私钥 = (tweak + 父私钥) mod n
    // 在secp256k1中通过 ec_seckey_tweak_add 实现
    unsigned char tweaked_key[32];
    std::copy(parent.key.begin(), parent.key.begin() + 32, (std::byte*)tweaked_key);
    
    // 使用secp256k1对私钥进行tweak
    if (!secp256k1_ec_seckey_tweak_add(
            secp256k1_context_static,
            tweaked_key,
            vout)) {
        throw std::runtime_error("secp256k1_ec_seckey_tweak_add failed");
    }
    
    // 构造子密钥
    child.key.Set(tweaked_key, tweaked_key + 32, true);
    if (!true) {
        throw std::runtime_error("Failed to construct child key");
    }
    
    if (!child.key.IsValid()) {
        throw std::runtime_error("Invalid child key generated");
    }
    
    return child;
}

/**
 * 按派生路径派生多个密钥
 * 
 * 派生路径格式: /86h/1h/0h/0/*
 * 或直接使用: /86h/1h/0h/0/
 * 后面跟索引列表 0, 1, 2, ..., range-1
 */
std::vector<ExtendedPrivateKey> DeriveKeys(
    const ExtendedPrivateKey& master,
    const std::string& path_str,
    uint32_t range)
{
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    std::cout << "Step 3: BIP32密钥派生" << std::endl;
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    
    std::cout << "派生路径: m" << path_str << "*" << std::endl;
    std::cout << "派生范围: 0-" << (range - 1) << " (" << range << " 个密钥)" << std::endl;
    std::cout << std::endl;
    
    // 解析路径，去掉开头的/和结尾的/或*
    std::string path = path_str;
    if (!path.empty() && path[0] == '/') path = path.substr(1);
    if (!path.empty() && (path.back() == '/' || path.back() == '*')) path.pop_back();
    
    // 分割路径
    std::vector<DerivationIndex> indices;
    size_t pos = 0;
    while (pos < path.length()) {
        size_t next_slash = path.find('/', pos);
        if (next_slash == std::string::npos) next_slash = path.length();
        
        std::string part = path.substr(pos, next_slash - pos);
        if (!part.empty() && part != "*") {
            bool is_hardened = !part.empty() && part.back() == 'h';
            if (is_hardened) part.pop_back();
            
            uint32_t index = std::stoul(part);
            indices.push_back({index, is_hardened});
        }
        
        pos = next_slash + 1;
    }
    
    // 从主密钥开始派生
    ExtendedPrivateKey current = master;
    
    std::cout << "派生步骤:" << std::endl;
    std::cout << "  m (主密钥)" << std::endl;
    
    for (const auto& index : indices) {
        current = DeriveChildKey(current, index.GetValue());
        std::cout << "    ↓ /" << index.ToString();
    }
    std::cout << std::endl << std::endl;
    
    // 现在派生从0到range-1的子密钥
    std::vector<ExtendedPrivateKey> derived_keys;
    
    std::cout << "派生 " << range << " 个地址密钥:" << std::endl;
    
    for (uint32_t i = 0; i < range; i++) {
        ExtendedPrivateKey child = DeriveChildKey(current, i);
        derived_keys.push_back(child);
        
        // 只打印前5个和最后1个
        if (i < 5 || i == range - 1) {
            std::cout << "  [" << std::setw(4) << i << "] " 
                      << HexStr(std::span{child.key.begin(), child.key.begin() + 16}) << "..." << std::endl;
        } else if (i == 5) {
            std::cout << "  ... (" << (range - 6) << " more)" << std::endl;
        }
    }
    
    std::cout << std::endl << "✓ 派生完成: " << derived_keys.size() << " 个密钥" << std::endl;
    std::cout << std::endl;
    
    return derived_keys;
}

// ============================================================================
// Step 4: 生成Taproot地址
// ============================================================================

/**
 * 从私钥生成Taproot地址
 * 
 * 步骤：
 * 1. 从私钥获取压缩公钥
 * 2. 提取x坐标（第1-32字节），获得XOnlyPubKey
 * 3. 计算TapTweak: hash_TapTweak(xonly_pubkey || merkle_root)
 * 4. 添加tweak到xonly公钥，得到输出公钥
 * 5. 使用Bech32m编码生成地址 (bc1p...)
 */
std::string GenerateTaprootAddress(const ExtendedPrivateKey& key)
{
    // 获取压缩公钥
    CPubKey pubkey = key.key.GetPubKey();
    if (!pubkey.IsCompressed()) {
        throw std::runtime_error("Expected compressed public key");
    }
    
    // 从压缩公钥提取x坐标 (跳过前缀)
    // 压缩公钥格式: [02/03] [32字节x坐标]
    // 我们只需要x坐标部分（后32字节）
    XOnlyPubKey xonly(pubkey);
    
    // 对于Taproot keypath spend，计算output key = internal_key + TapTweak(internal_key)
    // 在没有脚本的情况下，merkle_root为nullptr
    auto tweaked_result = xonly.CreateTapTweak(nullptr);
    
    if (!tweaked_result.has_value()) {
        throw std::runtime_error("Failed to create Taproot tweak");
    }
    
    XOnlyPubKey output_key = tweaked_result->first;
    
    // 构造WitnessV1Taproot目标
    WitnessV1Taproot taproot_dest(output_key);
    
    // 将目标转换为地址字符串
    CTxDestination dest(taproot_dest);
    std::string address = EncodeDestination(dest);
    
    if (address.empty()) {
        throw std::runtime_error("Failed to encode Taproot address");
    }
    
    return address;
}

/**
 * 为一批派生的密钥生成Taproot地址
 */
std::vector<std::string> GenerateTaprootAddresses(
    const std::vector<ExtendedPrivateKey>& keys)
{
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    std::cout << "Step 4: 生成Taproot地址 (bc1p...)" << std::endl;
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    
    std::vector<std::string> addresses;
    
    std::cout << "生成地址数: " << keys.size() << std::endl;
    std::cout << std::endl;
    
    std::cout << "示例地址 (前5个):" << std::endl;
    
    for (size_t i = 0; i < keys.size(); i++) {
        std::string addr = GenerateTaprootAddress(keys[i]);
        addresses.push_back(addr);
        
        if (i < 5) {
            std::cout << "  [" << std::setw(4) << i << "] " << addr << std::endl;
        } else if (i == 5) {
            std::cout << "  ... (" << (keys.size() - 5) << " more)" << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "✓ 地址生成完成: " << addresses.size() << " 个Taproot地址" << std::endl;
    std::cout << std::endl;
    
    return addresses;
}

// ============================================================================
// Step 5: 扫描UTXO (RPC调用)
// ============================================================================

/**
 * 构建scantxoutset RPC命令
 * 
 * 命令格式:
 * bitcoin-cli -signet scantxoutset start '["addr(address1)", "addr(address2)", ...]'
 */
std::string BuildScanTxOutSetCommand(const std::span<const std::string>& addresses)
{
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    std::cout << "Step 5: 构建UTXO扫描命令" << std::endl;
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    
    std::string command = "bitcoin-cli -signet -datadir=/home/jun/bitcoin/signet-wallet-1-balance-hongjunliao/data/0 scantxoutset start '[";
    
    for (size_t i = 0; i < addresses.size()/* addresses.size() */; i++) {
        command += "\"addr(" + addresses[i] + ")\",";
    }
    
    command[command.size() - 1] = ']'; // 替换最后一个逗号为]"]'";
    command += "'";
    
    std::cout << "RPC命令:" << std::endl;
    std::cout << command << std::endl;
    std::cout << "  扫描地址数: " << addresses.size() << std::endl;
    std::cout << std::endl;
    
    return command;
}

std::string executeCommandtxoutset(const std::string& command)
{
    FILE *fp = popen(command.c_str(), "r");
    if (!fp) return "";

    std::string out;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
    {
        out += std::string(buf, n);
    }
    pclose(fp);
    return out;
}

// ============================================================================
// Step 6: 解析RPC结果并输出余额
// ============================================================================

/**
 * 模拟RPC结果解析
 * 实际应该解析JSON响应
 */
void PrintBalanceResult(
    const std::vector<std::string>& addresses,
    size_t utxo_count,
    CAmount total_sats)
{
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    std::cout << "Step 6: 解析扫描结果" << std::endl;
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    
    std::cout << "扫描统计:" << std::endl;
    std::cout << "  地址数: " << addresses.size() << std::endl;
    std::cout << "  发现UTXO数: " << utxo_count << std::endl;
    std::cout << std::endl;
    
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    std::cout << "最终余额" << std::endl;
    std::cout << "════════════════════════════════════════════════════════" << std::endl;
    
    double balance_btc = static_cast<double>(total_sats) / COIN;
    
    std::cout << "总余额: " << std::fixed << std::setprecision(8) 
              << balance_btc << " BTC" << std::endl;
    std::cout << "总余额: " << total_sats << " sats" << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// 主测试用例
// ============================================================================

BOOST_AUTO_TEST_CASE(test_complete_taproot_descriptor_balance)
{
    // 选择Signet网络
    SelectParams(ChainType::SIGNET);
    ECC_Context ctx;
    
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Bitcoin Core Taproot Descriptor 余额查询 (完整版)    ║" << std::endl;
    std::cout << "║  Network: Signet                                     ║" << std::endl;
    std::cout << "║  Protocol: BIP32, BIP341, BIP342                     ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n";
    
    try {
        // 测试Descriptor
        const std::string DESCRIPTOR = 
            "tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5";
        
        // ========== Step 1: 解析Descriptor ==========
        auto [ext_key_str, path_str, checksum] = ParseDescriptorString(DESCRIPTOR);
        
        // ========== Step 2: 解码扩展私钥 ==========
        ExtendedPrivateKey master_key = DecodeExtendedKey(ext_key_str);
        BOOST_REQUIRE(master_key.IsValid());
        BOOST_REQUIRE(master_key.key.IsValid());
        
        // ========== Step 3: 派生子密钥 ==========
        const uint32_t DERIVATION_RANGE = 2000;
        std::vector<ExtendedPrivateKey> derived_keys = DeriveKeys(
            master_key, 
            path_str, 
            DERIVATION_RANGE);
        BOOST_REQUIRE_EQUAL(derived_keys.size(), DERIVATION_RANGE);
        
        // ========== Step 4: 生成Taproot地址 ==========
        std::vector<std::string> addresses = GenerateTaprootAddresses(derived_keys);
        BOOST_REQUIRE_EQUAL(addresses.size(), DERIVATION_RANGE);
        
        // 验证地址格式
        for (const auto& addr : addresses) {
            // Taproot地址应该以 tb1p (testnet) 或 bc1p (mainnet) 开头
            BOOST_CHECK(addr.find("tb1p") == 0 || addr.find("bc1p") == 0);
            BOOST_CHECK_EQUAL(addr.length(), 62);  // Bech32m格式长度
        }
        
        // ========== Step 5: 构建RPC命令 ==========
        double total = 0.0;

        for(int i = 0; i < addresses.size(); i += 200){
            auto addrs = std::span{addresses}.subspan(i, 200);
			std::string rpc_command = BuildScanTxOutSetCommand(addrs);

			std::string ret_json = executeCommandtxoutset(rpc_command);
			UniValue result(UniValue::VOBJ);
			auto r_json = result.read(ret_json);
			std::cout << "RPC return JSON:" << "|" << ret_json << "|" << ( r_json? result.write() : "invalid json") << std::endl;
			const UniValue& total_amount{result.find_value("total_amount")};
			BOOST_CHECK(!total_amount.isNull() && total_amount.isNum());

			total += total_amount.get_real();
        }
        printf("wallet_001: %.8f\n", total);
        return;
        // ========== Step 6: 模拟结果 ==========
        // 实际环境中应该执行RPC并解析JSON响应
        size_t utxo_count = 0;        // 演示值，实际从RPC获取
        CAmount total_sats = 0;       // 演示值，实际从RPC获取
        
        PrintBalanceResult(addresses, utxo_count, total_sats);
        
        std::cout << "═══════════════════════════════════════════════════════" << std::endl;
        std::cout << "✓ 测试完成" << std::endl;
        std::cout << "═══════════════════════════════════════════════════════" << std::endl;
        std::cout << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "✗ 错误: " << e.what() << std::endl;
        BOOST_ERROR(e.what());
    }
}

// ============================================================================
// 单步测试用例 (用于调试)
// ============================================================================

/**
 * 仅测试Descriptor解析
 */
BOOST_AUTO_TEST_CASE(test_descriptor_parsing)
{
    SelectParams(ChainType::SIGNET);
    
    const std::string DESCRIPTOR = 
        "tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5";
    
    auto [ext_key, path, checksum] = ParseDescriptorString(DESCRIPTOR);
    
    BOOST_CHECK_EQUAL(ext_key, "tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ");
    BOOST_CHECK_EQUAL(path, "/86h/1h/0h/0/");
    BOOST_CHECK_EQUAL(checksum, "twn4yrj5");
}

/**
 * 仅测试扩展密钥解码
 */
BOOST_AUTO_TEST_CASE(test_extended_key_decoding)
{
    SelectParams(ChainType::SIGNET);
    
    const std::string EXT_KEY = "tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ";
    
    ExtendedPrivateKey key = DecodeExtendedKey(EXT_KEY);
    
    BOOST_CHECK(key.IsValid());
    BOOST_CHECK(key.key.IsValid());
    BOOST_CHECK_EQUAL(key.chaincode.size(), 32);
}

/**
 * 仅测试密钥派生
 */
BOOST_AUTO_TEST_CASE(test_bip32_derivation)
{
    SelectParams(ChainType::SIGNET);
    
    const std::string EXT_KEY = "tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ";
    ExtendedPrivateKey master = DecodeExtendedKey(EXT_KEY);
    
    // 派生一个子密钥
    const uint32_t HARDENED_86 = 0x80000056;  // 86h
    ExtendedPrivateKey child = DeriveChildKey(master, HARDENED_86);
    
    BOOST_CHECK(child.IsValid());
    BOOST_CHECK_EQUAL(child.nDepth, master.nDepth + 1);
    BOOST_CHECK(child.key != master.key);  // 应该是不同的密钥
}

/**
 * 仅测试Taproot地址生成
 */
BOOST_AUTO_TEST_CASE(test_taproot_address_generation)
{
    SelectParams(ChainType::SIGNET);
    
    const std::string EXT_KEY = "tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ";
    ExtendedPrivateKey master = DecodeExtendedKey(EXT_KEY);
    
    // 派生第一个地址密钥
    ExtendedPrivateKey address_key = DeriveChildKey(master, 0);
    
    // 生成地址
    std::string address = GenerateTaprootAddress(address_key);
    
    // 验证地址格式
    BOOST_CHECK(!address.empty());
    BOOST_CHECK(address.find("tb1p") == 0);  // Signet Taproot地址
    BOOST_CHECK_EQUAL(address.length(), 62); // Bech32m长度
}

BOOST_AUTO_TEST_SUITE_END()
