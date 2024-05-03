 /*!
 * @author hongjun.liao <docici@126.com>, @date 2023/6/29
 *
 * rebuild the old Bitcoin code(Bitcoin v0.1.5) on a modern Linux/Ubuntu distribution
 * */

// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

/////////////////////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "hp/hp_assert.h"
#include "hp/hp_config.h"
#include <string.h>
/////////////////////////////////////////////////////////////////////////////////////////

int btc_inih_handler(void* user, const char* section, const char* name,const char* value)
{
	assert(user);
	hp_ini * ini = (hp_ini*)user;
	dict* d = ini->dict;
	assert(d);

	dictReplace(d, sdsnew(name), sdsnew(value));

	if(strcmp(name, "addr") == 0){
		/* addr=137.134.23.25:8339 */
		char bind_[128] = "";
		int port_ = 0;

		if(value && strlen(value) > 0){
			int n = sscanf(value, "%[^:]:%d", bind_, &port_);
			if(n != 2){
				return 0;
			}
		}
		dictReplace(d, sdsnew("btc.bind"), sdsnew(bind_));
		dictReplace(d, sdsnew("btc.port"), sdsfromlonglong(port_));
	}

	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
static hp_ini definiobj = {.parser = btc_inih_handler };
//global default configure
hp_ini * g_defini = &definiobj;

#ifndef NDEBUG
static hp_ini deftestiniobj = {.parser = btc_inih_handler };
//global default configure
hp_ini * g_deftestini = &deftestiniobj;
#endif
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
int test_btc_config_main(int argc, char ** argv)
{
	hp_ini testiniobj = {.parser = btc_inih_handler }, * testini = &testiniobj;
#define cfg(k) hp_config_ini(testini, (k))
#define cfgi(k) atoi(cfg(k))
	assert(cfgi("#set test.key.name 23") == 0 && cfgi("test.key.name") == 23);
	assert(cfgi("#set test.key.name 24") == 0 && cfgi("test.key.name") == 24);
	hp_assert(cfgi("#load bitcoin.conf") == 0, "'#load bitcoin.conf' failed");
	hp_assert(cfgi("#load this_file_not_exist.conf") != 0, "'#load this_file_not_exist.conf' OK?");
	hp_assert(strlen(cfg("loglevel")) > 0, "loglevel NOT found");
	hp_assert(strlen(cfg("#show")) > 0, "#show failed");

	return 0;
}


#endif //NDEBUG

/////////////////////////////////////////////////////////////////////////////////////////
