/*!
 * @author hongjun.liao <docici@126.com>, @date 2023/6/29
 *
 * rebuild the old Bitcoin code(Bitcoin v0.1.5) on a modern Linux/Ubuntu distribution
 * */

// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

/////////////////////////////////////////////////////////////////////////////////////////////


#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>     /* assert */
#include "hp/hp_pub.h"
#include "hp/hp_io_t.h"      /* ,... */
#include "hp/hp_log.h"
#include "hp/str_dump.h"
#include "net.h"

#include "hp/hp_http.h"
#include "hp/hp_net.h"
extern "C"{
#include "redis/src/dict.h"	  	/* dict */
#include "redis/src/adlist.h"	/* list */
}

/////////////////////////////////////////////////////////////////////////////////////////

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
/**
 * BTC protocol
 * */

static int btc_node_io_send(btc_node * io, btc_node_msghdr * rmsg, int flags)
{
	int rc;
//	if(!(io && rmsg))
//		return -1;
//
//	btc_node_io_ctx * ioctx = io->ioctx;
//	sds topic = sdsnew(redis_cli_topic(rmsg->topic));
//	int len = sdslen(rmsg->payload);
//
//	uint16_t message_id = ++ioctx->mqid;
//	if(ioctx->mqid + 1 == UINT16_MAX)
//		ioctx->mqid = 0;
//
//	uint16_t netbytes;
//	uint16_t topic_len = sdslen(topic);
//
//	size_t total_len = 2 + topic_len + len;
//	if (MG_MQTT_GET_QOS(flags) > 0) {
//		total_len += 2;
//	}
//
//	btc_node_io_send_header(io, MG_MQTT_CMD_PUBLISH, flags, total_len);
//
//	netbytes = htons(topic_len);
//	hp_io_write((hp_io_t *)io, &netbytes, 2, (void *)-1, 0);
//	rc = hp_io_write((hp_io_t *)io, (void *)(topic), topic_len, (hp_io_free_t)sdsfree, 0);
//
//	if (MG_MQTT_GET_QOS(flags) > 0) {
//		netbytes = htons(message_id);
//		hp_io_write((hp_io_t *)io, &netbytes, 2, (void *)-1, 0);
//	}
//
//	rc = hp_io_write((hp_io_t *)io, (void *)sdsdup(rmsg->payload), len, (hp_io_free_t)sdsfree, 0);
//	/* for ACK */
//	io->l_mid = message_id;
//
//	if(hp_log_level > 7){
//		int len = sdslen(rmsg->payload);
//		hp_log(stdout, "%s: fd=%d, sending topic='%s', msgid/QOS=%u/%u, playload=%u/'%s'\n", __FUNCTION__
//				, ((hp_io_t *)io)->fd, topic, io->l_mid, 2, len, dumpstr(rmsg->payload, len, 64));
//	}

	return rc;
}

/**
*/
//static void * sdslist_dup(void *ptr)
//{
//	libim_msg_t * msg = (libim_msg_t *)ptr;
//	assert(ptr);
//
//	libim_msg_t * ret = calloc(1, sizeof(libim_msg_t));
//	ret->payload = sdsdup(msg->payload);
//	ret->mid = sdsdup(msg->mid);
//	ret->topic = sdsdup(msg->topic);
////	ret->sid = sdsdup(msg->sid);
//
//	return ret;
//}

static void sdslist_free(void *ptr)
{
	btc_node_msghdr * msg = (btc_node_msghdr *)ptr;
	assert(ptr);

//	sdsfree(msg->payload);
//	sdsfree(msg->mid);
//	sdsfree(msg->topic);
//	sdsfree(msg->sid);

	free(ptr);
}

static int sdslist_match(void *ptr, void *key)
{
//	assert(ptr);
//	btc_node_rmsg_t * msg = (btc_node_rmsg_t *)ptr;
//	assert(strlen(msg->mid) > 0);
//	assert(strlen((char *)key) > 0);
//
//	return strncmp(msg->mid, (char *)key, sdslen(msg->mid)) == 0;
}

int btc_node_init(btc_node * node, btc_node_ctx * bctx)
{
	if(!(node && bctx))
		return -1;

	memset(node, 0, sizeof(btc_node));

//	io->sid = sdsnew("");
	node->bctx = bctx;
//	io->qos = dictCreate(&qosTableDictType, NULL);
//
//	io->outlist = listCreate();
////	listSetDupMethod(io->outlist, sdslist_dup);
//	listSetFreeMethod(io->outlist, sdslist_free);
//	listSetMatchMethod(io->outlist, sdslist_match);

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

//	dictRelease(io->qos);
//	listRelease(io->outlist);
//	sdsfree(io->sid);
}


static int btc_node_loop(btc_node * io)
{
	assert(io && io->bctx);
	int rc = 0;
goto exit_;
//	btc_node_ctx * ioctx = io->bctx;
//	btc_node_msghdr * rmsg = 0;
exit_:
	return rc;
}

/////////////////////////////////////////////////////////////////////////////////////////

static hp_io_t *  btc_node_on_new(hp_io_t * cio, hp_sock_t fd)
{
	assert(cio && cio->user);

	btc_node_ctx * bctx = (btc_node_ctx *)cio->user;
	if(!bctx) { return 0; }
	btc_node * node = (btc_node *)calloc(1, sizeof(btc_node));
	int rc = btc_node_init(node, bctx);
	assert(rc == 0);

	listAddNodeTail(bctx->nodes, node);

	if(hp_log_level > 0){
		char buf[64] = "";
		hp_log(stdout, "%s: new BTC connection from '%s', IO total=%d\n", __FUNCTION__, hp_get_ipport_cstr(fd, buf),
				btc_node_ctx_count(bctx));
	}
	return (hp_io_t *)node;
}

static int btc_node_on_parse(hp_io_t * io, char * buf, size_t * len
	, hp_iohdr_t ** hdrp, char ** bodyp)
{
	return 0; //btc_node_parse(buf, len, flags, hdrp, bodyp);
}

static int btc_node_on_dispatch(hp_io_t * io, hp_iohdr_t * imhdr, char * body)
{
	return 0; //btc_node_dispatch((btc_node_io_t *)io, imhdr, body);
}

static int btc_node_on_loop(hp_io_t * io)
{
	return btc_node_loop((btc_node *)io);
}

static void btc_node_on_delete(hp_io_t * io)
{
	btc_node * node = (btc_node *)io;
	assert(node && node->bctx);
	btc_node_ctx * bctx = node->bctx;

	if(hp_log_level > 0){
		char buf[64] = "";
		hp_log(stdout, "%s: delete BTC connection '%s', IO total=%d\n", __FUNCTION__
				, hp_get_ipport_cstr(hp_io_fd(io), buf), btc_node_ctx_count(bctx));
	}

	listDelNode(bctx->nodes, 0);

	btc_node_uninit(node);
	free(node);

}

/* callbacks for btc_node clients */
static hp_iohdl s_btc_nodehdl = {
	.on_new = btc_node_on_new,
	.on_parse = btc_node_on_parse,
	.on_dispatch = btc_node_on_dispatch,
	.on_delete = btc_node_on_delete,
	.on_loop = btc_node_on_loop
};

int btc_node_ctx_init(btc_node_ctx * bctx
	, hp_io_ctx * ioctx
	, hp_sock_t fd, int tcp_keepalive
	, int ping_interval)
{
	int rc;
	if (!(bctx && ioctx)) { return -1; }

	bctx->ioctx = ioctx;
	bctx->rping_interval = ping_interval;

	rc = hp_io_add(bctx->ioctx, &bctx->listenio, fd, s_btc_nodehdl);
	if (rc != 0) { return -4; }

	bctx->nodes = listCreate();
	bctx->listenio.user = bctx;
	bctx->listenio.iohdl.on_loop = 0;

	return rc;
}

void btc_node_ctx_uninit(btc_node_ctx * bctx)
{
	listRelease(bctx->nodes);
}

/////////////////////////////////////////////////////////////////////////////////////////
btc_node * btc_node_find(btc_node_ctx * bctx, int (* match)(void *ptr, void *key))
{
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
int btc_http_process(struct hp_http * http, hp_httpreq * req, struct hp_httpresp * resp)
{
	assert(http && req && resp);

	resp->status_code = 200;
	resp->flags = 0;
	resp->html = req->url;

	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
