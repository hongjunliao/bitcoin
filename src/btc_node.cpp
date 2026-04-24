/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////

#include "btc_node.h"
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>     /* assert */
#include <arpa/inet.h>
#include <hp/sdsinc.h>
extern "C"{
#include "redis/src/dict.h"	  	/* dict */
#include "redis/src/adlist.h"	/* list */
}
/////////////////////////////////////////////////////////////////////////////////////////

/*====================== Hash table type implementation  ==================== */
static int r_dictSdsKeyCompare(dict *d, const void *key1, const void *key2)
{
    int l1,l2;
//    DICT_NOTUSED(privdata);

    l1 = sdslen((sds)key1);
    l2 = sdslen((sds)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

static void r_dictSdsDestructor(dict *d, void *obj)
{
//    DICT_NOTUSED(privdata);

    sdsfree((sds)obj);
}

static uint64_t r_dictSdsHash(const void *key) {
    return dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
}


/* QOS table. sds string -> QOS int */
static dictType qosTableDictType = {
	r_dictSdsHash,            /* hash function */
    NULL,                   /* key dup */
    NULL,                   /* val dup */
	r_dictSdsKeyCompare,      /* key compare */
    r_dictSdsDestructor,      /* key destructor */
	NULL                    /* val destructor */
};


/////////////////////////////////////////////////////////////////////////////////////////

int btc_node_init(btc_node * node, btc_node_ctx * bctx)
{
	if(!(node && bctx))
		return -1;

	node->bctx = bctx;

	hp_iohdl hdl = bctx->listenio.iohdl;
	hdl.on_new = 0;
	node->io.iohdl = hdl;

	return 0;
}



void btc_node_uninit(btc_node * node)
{
	if(!node)
		return ;

	int rc;
	btc_node_ctx * ioctx = node->bctx;
}

/////////////////////////////////////////////////////////////////////////////////////////

int btc_init(btc_node_ctx * bctx
	, hp_io_ctx * ioctx, hp_iohdl hdl
	, hp_sock_t fd, int tcp_keepalive
	, int ping_interval)
{
	int rc;
	if (!(bctx && ioctx)) { return -1; }

	bctx->ioctx = ioctx;
	bctx->rping_interval = ping_interval;

	rc = hp_io_add(bctx->ioctx, &bctx->listenio, fd, hdl);
	if (rc != 0) { return -4; }

	bctx->inlist = listCreate();
	bctx->outlist = listCreate();

	bctx->listenio.user = bctx;

	return rc;
}

void btc_uninit(btc_node_ctx * bctx)
{
	listRelease(bctx->inlist);
	listRelease(bctx->outlist);
}

/////////////////////////////////////////////////////////////////////////////////////////

btc_node * btc_out_find(btc_node_ctx * bctx, void * key, int (* match)(void *ptr, void *key))
{
	list li = { .match = bctx->outlist->match };
	listSetMatchMethod(bctx->outlist, match);
	listNode * node = listSearchKey(bctx->outlist, key);
	listSetMatchMethod(bctx->outlist, li.match);

	return (btc_node *)(node? listNodeValue(node) : 0);
}


/////////////////////////////////////////////////////////////////////////////////////////
