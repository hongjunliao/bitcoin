/*!
 * @author hongjun.liao <docici@126.com>
 *
 * 区块通过引用上一个区块的hash形成链
 *
 * 0.一开始,大家第一个区块是固定的: 即"genesis block"
 * 1.通过p2p消息"get latest block"查询最新区块
 * 2.通过将交易打包到区块并广播,表示自己挖出了最新区块
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
//
#include "config.h"
#include "bitcoinkernel.h"
#include "hp/sdsinc.h"
#include <boost/test/unit_test.hpp>
#include "hp/hp_ini.h"
#include "hp/hp_str.h"
extern hp_ini * g_ini;
#define cfg(k) hp_ini_exec(g_ini, (k))

#define GENESIS_HASH "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"

static int btck_WriteBytes_cb(const void* bytes, size_t size, void* userdata) {
    std::cout << dumpstr((char const *)bytes, size, size) << std::endl;
    return 0;
}

int btc_get_genesis_hash(sds datadir, sds blockdir)
{
    int rc;

    auto copts = btck_context_options_create();
    auto ctx = btck_context_create(copts);
    auto mgropts = btck_chainstate_manager_options_create(ctx, datadir, sdslen(datadir), blockdir, sdslen(blockdir));
    auto mgr = btck_chainstate_manager_create(mgropts);
    auto chain = btck_chainstate_manager_get_active_chain(mgr);
    auto entry = btck_chain_get_by_height(chain, 0);
    auto block = btck_block_read(mgr, entry);
    auto count = btck_block_count_transactions(block);
    assert(count == 1);
    auto tran = btck_block_get_transaction_at(block, 0);
    auto vin = btck_transaction_get_input_at(tran, 0);
    rc = btck_transaction_input_get_script_sig(vin, btck_WriteBytes_cb, 0);
    assert(rc == 0);

    auto blockh = btck_block_get_hash(block);
    unsigned char h[32]{""};
    btck_block_hash_to_bytes(blockh, h);

    btck_block_destroy(block);
    btck_chainstate_manager_destroy(mgr);
    btck_chainstate_manager_options_destroy(mgropts);
    btck_context_destroy(ctx);
    btck_context_options_destroy(copts);

    return 0;
}

BOOST_AUTO_TEST_SUITE(block)

BOOST_AUTO_TEST_CASE(genesis) {
    sds datadir = sdsnew(cfg("datadir")), blockdir = sdscatprintf(sdsempty(), "%s/blocks/", cfg("datadir"));
    auto genesis = btc_get_genesis_hash(datadir, blockdir);
    sdsfree(datadir);
    sdsfree(blockdir);
}

BOOST_AUTO_TEST_SUITE_END()
/////////////////////////////////////////////////////////////////////////////////////////////
