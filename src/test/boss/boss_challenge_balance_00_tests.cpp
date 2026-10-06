//写一个完整的bitcoin core c/c++测试：内容为 查询desriptor:tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5 中的余额;实现方式:你需要解析该descriptor，得到private key,chaincode,然后derive等等全部流程， 最后通过调用biltcoin-cli rpc scantxoutset，传入你生成的addr，最终输出余额；bitcoind已在本地同步， -signet, range为2000以内
#include <iostream>
#include <vector>
#include <string>

#include <key.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <util/strencodings.h>
#include <key_io.h>

#include <interfaces/node.h>

using namespace std;

static int openai_main() {
    // descriptor
    const std::string desc_str =
        "tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5";

    FlatSigningProvider provider;
    std::string error;

    // 解析 descriptor
    auto descs = Parse(desc_str, provider, error);
    if (descs.empty()) {
        std::cerr << "Parse error: " << error << std::endl;
        return 1;
    }

    auto& desc = descs[0];

    // range
    int range = 2000;

    for (int i = 0; i < range; ++i) {
        std::vector<CScript> scripts;

        if (!desc->Expand(i, provider, scripts, provider)) {
            std::cerr << "Expand failed at index " << i << std::endl;
            continue;
        }

        for (const auto& script : scripts) {
            // 转地址
            CTxDestination dest;
            if (!ExtractDestination(script, dest)) continue;

            std::string addr = EncodeDestination(dest);

            std::cout << "Address[" << i << "]: " << addr << std::endl;

            // 调用 bitcoin-cli
            std::string cmd =
                "bitcoin-cli -signet scantxoutset start "
                "'[{\"desc\":\"addr(" + addr + ")\"}]'";

            std::cout << "Running: " << cmd << std::endl;
            system(cmd.c_str());
        }
    }

    return 0;
}