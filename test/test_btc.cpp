/*!
 * @author hongjun.liao <docici@126.com>, @date 2023/6/29
 *
 * rebuild the old Bitcoin code(Bitcoin v0.1.5) on a modern Linux/Ubuntu distribution
 * */

// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BTC_TEST_H
#define BTC_TEST_H

#include "config.h"

#include "hp/hp_log.h"
#include <cassert>
#include <stdexcept>
#include "../src/headers.h"
#include "../src/bignum.h"
#include "../src/uint256.h"
#include "../src/util.h"
#include "../src/net.h"
#include "../src/key.h"
#include "../src/db.h"
#include "../src/base58.h"
#include "../src/script.h"
#include "hp/libhp.h"
#include "hp/hp_redis.h"
#include "hp/hp_config.h"
#include "sds/sds.h"
/////////////////////////////////////////////////////////////////////////////////////////////
extern hp_config_t g_config;
#define cfg g_config
#define cfgi(k) atoi(cfg(k))
int test_btc_config_main(int argc, char ** argv);
/////////////////////////////////////////////////////////////////////////////////////////////

int test_btc_main(int argc, char ** argv)
{
	int rc;
	hp_log(stdout, "%s...\n", __FUNCTION__);
	{
		if(cfgi("libhp.runtest")){
			rc = libhp_all_tests_main(argc, argv); assert(rc == 0);
		}
	}
	if(!cfgi("btc.runtest"))
		goto done;
	cfg("#load bitcoin.conf");
	{
		rc = test_btc_config_main(argc, argv); assert(rc == 0);
	}
	//base58.h
	{
		char buf[] = "1iIl0O";
		string s1 = EncodeBase58((unsigned char *)buf, (unsigned char *)(buf + sizeof(buf)));
		hp_log(stdout, "%s:s/EncodeBase58='%s'/'%s'\n", __FUNCTION__,buf, s1.c_str());
	}
	{
	    string str = strprintf("%I64d.%0I64d.%0xI64d.%08I64d.%8I64d", 22222, 22222, 22222, 22222, 22222);
	    hp_log(stdout, "%s: str='%s'\n", __FUNCTION__, str.c_str());
	}
	{
		CBigNum a = 1, b = 2, c = a + b, d, e;
		d.setint64(4);
		assert(c==3);
		CDataStream ds;
		ds << d;
		ds >> e;
		assert(e == 4);
	}
	{
		uint256 a("10"), b("20"), c = a + b, d("30"), e("31");
		assert(c == d && c != e);
	}
	{
		AddTimeData(1, 1);
		string s = strprintf("int=%d,string=%s", 1, string("hello").c_str());
		assert(s == "int=1,string=hello");
	}
	{
		CMessageHeader hdr("publish", 0), hdr2, hdr3;
		CDataStream s;
		s << hdr;
		s >> hdr2;

		assert(hdr.IsValid() && hdr2.IsValid() && !hdr3.IsValid());
	}
	{
		CAddress addr, addr2("192.168.1.2"), addr3;
		assert(addr.ToStringIP() == "0.0.0.0");
		assert(addr2.ToStringIP() == "192.168.1.2");
		CDataStream s;
		s << addr2;
		s >> addr3;
		assert(addr3.ToStringIP() == "192.168.1.2");
		assert(addr2 == addr3);

	}
	{
		CKey key;
		key.MakeNewKey();
		uint256 hashdata("hello");
		std::vector<unsigned char> vchsig;
		bool f = key.Sign(hashdata, vchsig);
		bool f2 = key.Verify(hashdata, vchsig);
		assert(!vchsig.empty() && f && f2);
	}
	{
		LoadWallet();
		SetAddressBookName("192.168.1.1", "host1");
	}
	cfg("#unload");
done:
	hp_log(stdout, "%s done\n", __FUNCTION__);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////

#endif //BTC_TEST_H
