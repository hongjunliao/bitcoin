// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <test/data/base58_encode_decode.json.h>

#include <base58.h>
#include <test/util/json.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <util/vector.h>

#include <univalue.h>
#include <script/descriptor.h>

#include <boost/test/unit_test.hpp>

#include <key_io.h>

using namespace std::literals;
using namespace util::hex_literals;

const std::string BOSS_DESC =
"tr(tprv8ZgxMBicQKsPeUtaZbdhCrgvRSB6G6XKwAzsH1AiiGNZnvMvVFmtkK3Fd3YFVmYeXAA3F5jLMR6xynRhFdM8vH8u8GESS4d7nrwTXPQp9ZJ/86h/1h/0h/0/*)#twn4yrj5";

struct SignetSetup : public TestingSetup {
	SignetSetup()
        : TestingSetup{ChainType::SIGNET} {}
};

BOOST_FIXTURE_TEST_SUITE(boss_challenge_balance_tests, SignetSetup)

BOOST_AUTO_TEST_CASE(boss_challenge_balance)
{
    FlatSigningProvider keys_priv, keys_pub;
    std::string error;

    std::vector<std::unique_ptr<Descriptor>> parse_privs;
    parse_privs = Parse(BOSS_DESC, keys_priv, error, true);
    BOOST_CHECK_MESSAGE(!parse_privs.empty(), error);
}


BOOST_AUTO_TEST_SUITE_END()
