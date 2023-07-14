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
#include "hp/sdsinc.h"
#include "hp/hp_config.h"
#include "inih/ini.h"		//ini_parse
extern "C"{
#include "redis/src/dict.h" //dict
}
#include <string.h>
/////////////////////////////////////////////////////////////////////////////////////////
/*====================== Hash table type implementation  ==================== */
static int r_dictSdsKeyCompare(void *privdata, const void *key1,  const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = sdslen((sds)key1);
    l2 = sdslen((sds)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

static void r_dictSdsDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);

    sdsfree((sds)val);
}

static uint64_t r_dictSdsHash(const void *key) {
    return dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
}

/////////////////////////////////////////////////////////////////////////////////////////////

/* config table. char * => char * */
static dictType configTableDictType = {
	r_dictSdsHash,            /* hash function */
    NULL,                   /* key dup */
    NULL,                   /* val dup */
	r_dictSdsKeyCompare,      /* key compare */
    r_dictSdsDestructor,      /* key destructor */
	r_dictSdsDestructor       /* val destructor */
};


static int inih_handler(void* user, const char* section, const char* name,
                   const char* value)
{
	dict* cfg = (dict*)user;
	assert(cfg);

	dictAdd(cfg, sdsnew(name), sdsnew(value));

	return 1;
}

static char const * btc_config_load(char const * id)
{
	if(!id) return 0;

	static dict * s_config = 0;
	if(!s_config){
		s_config = dictCreate(&configTableDictType, 0);
	}
	assert(s_config);

	int n = strlen("#load");
	if(strcmp(id, "#unload") == 0 && s_config){
		dictRelease(s_config);
		s_config = 0;
		return "0";
	}
	else if(strncmp(id, "#load", n) == 0 && strlen(id) >= (n + 2)){
		char const * f = id + n + 1;
		if (ini_parse(f, inih_handler, s_config) != 0) {
			hp_log(stderr, "%s: ini_parse failed for  '%s'\n", __FUNCTION__, f);
			return "-1";
		}
		return "0";
	}
	else if(strcmp(id, "#show") == 0){
		dictIterator * iter = dictGetIterator(s_config);
		dictEntry * ent;
		for(ent = 0; (ent = dictNext(iter));){
			printf("'%s'=>'%s'\n", (char *)ent->key, (char *)ent->v.val);
		}
		dictReleaseIterator(iter);
		return "0";
	}

	sds key = sdsnew(id);
	void * v = dictFetchValue(s_config, key);
	sdsfree(key);
	return v? (char *)v : "";
}

/////////////////////////////////////////////////////////////////////////////////////////////
hp_config_t g_config = btc_config_load;

/////////////////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
#define cfg g_config
#define cfgi(k) atoi(cfg(k))

int test_btc_config_main(int argc, char ** argv)
{
	hp_assert(cfgi("#load bitcoin.conf") == 0, "'#load bitcoin.conf' failed");
	hp_assert(cfgi("#load this_file_not_exist.conf") != 0, "'#load this_file_not_exist.conf' OK?");
	hp_assert(strlen(cfg("loglevel")) > 0, "loglevel NOT found");
	hp_assert(strlen(cfg("#show")) > 0, "#show failed");
	assert(cfgi("#unload") == 0);

	return 0;
}


#endif //NDEBUG

/////////////////////////////////////////////////////////////////////////////////////////
