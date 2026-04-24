/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <boost/test/unit_test.hpp>
#include "hp/hp_assert.h"
#include "hp/hp_ini.h"
#include <string.h>
/////////////////////////////////////////////////////////////////////////////////////////

int btc_inih_handler(void* user, const char* section, const char* name,const char* value)
{
	assert(user);
	hp_ini * ini = (hp_ini*)user;
	dict* d = ini->dict;
	assert(d);

	dictReplace(d, sdsnew(name), sdsnew(value));

	if(strcmp(name, "btc.p2p") == 0){
		/* addr=137.134.23.25:8339 */
		char bind_[128] = "";
		int port_ = 0;

		if(value && strlen(value) > 0){
			int n = sscanf(value, "%[^:]:%d", bind_, &port_);
			if(n < 1){
				return 0;
			}
		}
		dictReplace(d, sdsnew("btc.p2p.ip"), sdsnew(bind_));
		dictReplace(d, sdsnew("btc.p2p.port"), sdsfromlonglong(port_));
	}

	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(config)

BOOST_AUTO_TEST_CASE(config_file) {
	hp_ini testiniobj = {.parser = btc_inih_handler }, * testini = &testiniobj;
#define cfg(k) hp_ini_exec(testini, (k))
#define cfgi(k) atoi(cfg(k))
	assert(cfgi("#set test.key.name 23") == 0 && cfgi("test.key.name") == 23);
	assert(cfgi("#set test.key.name 24") == 0 && cfgi("test.key.name") == 24);
	hp_assert(cfgi("#load bitcoin.conf") == 0, "'#load bitcoin.conf' failed");
	hp_assert(cfgi("#load this_file_not_exist.conf") != 0, "'#load this_file_not_exist.conf' OK?");
	hp_assert(strlen(cfg("loglevel")) > 0, "loglevel NOT found");
	hp_assert(strlen(cfg("#show")) > 0, "#show failed");

}

BOOST_AUTO_TEST_SUITE_END()

/////////////////////////////////////////////////////////////////////////////////////////
