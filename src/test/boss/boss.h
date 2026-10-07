//
// Created by jun on 10/7/26.
//

#ifndef BITCOINCORE_BOSS_H
#define BITCOINCORE_BOSS_H

#include <boost/test/unit_test.hpp>
#include "test/util/setup_common.h"
#include "util/chaintype.h"

#define bcheck BOOST_CHECK
#define brequire BOOST_REQUIRE
#define bequal BOOST_CHECK_EQUAL


struct SignetSetup : public TestingSetup {
    SignetSetup()
        : TestingSetup{ChainType::SIGNET} {}
};

#endif //BITCOINCORE_BOSS_H
