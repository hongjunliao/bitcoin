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

#include "hp/hp_net.h"
extern "C"{
#include "redis/src/dict.h"	  	/* dict */
#include "redis/src/adlist.h"	/* list */
#include "sds/sds.h"			/* sds */

}

/////////////////////////////////////////////////////////////////////////////////////////
extern bool BitcoinMiner();

/////////////////////////////////////////////////////////////////////////////////////////

static hp_io_t *  btc_node_on_new(hp_io_ctx * ioctx, hp_sock_t confd)
{
	btc_node_io_ctx * c = (btc_node_io_ctx *)ioctx;
	if(!c) { return 0; }
	btc_node_io * node = (btc_node_io *)calloc(1, sizeof(btc_node_io));
	int rc = btc_node_io_init(node, c);
	assert(rc == 0);

	if (hp_log_level > 2) {
		char cliaddstr[64] = "";
		hp_log(stdout, "%s: new connect from '%s', fd=%d, total=%d\n"
			, __FUNCTION__, hp_get_ipport_cstr(confd, cliaddstr), confd, ++c->n_nodes);
	}
	return (hp_io_t *)node;
}

static int btc_node_on_parse(hp_io_t * io, char * buf, size_t * len, int flags
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
	return btc_node_io_loop((btc_node_io *)io);
}

static void btc_node_on_delete(hp_io_t * io)
{
	btc_node_io * node = (btc_node_io *)io;
	if(!(node && node->ioctx)) { return; }
	--node->ioctx->n_nodes;
	btc_node_io_uninit(node);
	free(node);

}

/* callbacks for btc_node clients */
static hp_iohdl s_btc_nodehdl = {
	.on_new = btc_node_on_new,
	.on_parse = btc_node_on_parse,
	.on_dispatch = btc_node_on_dispatch,
	.on_loop = btc_node_on_loop,
	.on_delete = btc_node_on_delete,
};

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

//    sdsfree((sds)val);
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

static int btc_node_io_send(btc_node_io * io, btc_node_msghdr * rmsg, int flags)
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

int btc_node_io_init(btc_node_io * io, btc_node_io_ctx * ioctx)
{
	if(!(io && ioctx))
		return -1;

	memset(io, 0, sizeof(btc_node_io));

//	io->sid = sdsnew("");
	io->ioctx = ioctx;
	io->qos = dictCreate(&qosTableDictType, NULL);

	io->outlist = listCreate();
//	listSetDupMethod(io->outlist, sdslist_dup);
	listSetFreeMethod(io->outlist, sdslist_free);
	listSetMatchMethod(io->outlist, sdslist_match);

	return 0;
}

int btc_node_io_append(btc_node_io * io, char const * topic, char const * mid, sds message, int flags)
{
	if(!(io && message))
		return -1;

	btc_node_msghdr * jmsg = (btc_node_msghdr *)calloc(1, sizeof(btc_node_msghdr));
//	jmsg->payload = sdsnew(message);
//	jmsg->topic = sdsnew(topic);
//	jmsg->mid = sdsnew(mid);

	list * li = listAddNodeTail(((btc_node_io *)io)->outlist, jmsg);
	assert(li);

	return 0;
}

void btc_node_io_uninit(btc_node_io * io)
{
	if(!io)
		return ;

	int rc;
	btc_node_io_ctx * ioctx = io->ioctx;

	dictRelease(io->qos);
	listRelease(io->outlist);
//	sdsfree(io->sid);
}

int btc_node_io_loop(btc_node_io * io)
{
	assert(io && io->ioctx);

	int rc = 0;
	btc_node_io_ctx * ioctx = io->ioctx;
	btc_node_msghdr * rmsg = 0;

	BitcoinMiner();

//	/* redis ping/pong */
//	if(ioctx->rping_interval > 0 && io->subc){
//
//		if(difftime(time(0), io->rping) > ioctx->rping_interval){
//
//			if(hp_log_level > 8)
//				hp_log(stdout, "%s: fd=%d, Redis PING ...\n", __FUNCTION__, ((hp_io_t *)io)->fd);
//
//			hp_sub_ping(io->subc);
//			io->rping = time(0);
//		}
//	}
//
//	/* check for current sending message */
//	if(io->l_msg){
//		rmsg = (btc_node_rmsg_t *)listNodeValue(io->l_msg);
//		assert(rmsg);
//
//		/* QOS > 0 need ACK */
//		dictEntry * ent = dictFind(io->qos, rmsg->topic);
//		int qos = (ent? ent->v.u64 : 2);
//
//		if(qos > 0){
//			/* check if current ACKed */
//			if(sdslen(rmsg->mid) > 0){
//				if(difftime(time(0), io->l_time) <= 10)
//					goto ret;
//
//				if(io->l_mid == 0){
//					/* resend */
//					rc = btc_node_io_send(io, rmsg, MG_MQTT_QOS(qos));
//					io->l_time = time(0);
//				}
//				goto ret;
//			}
//		}
//		else{ /* QOS=0 at most once */
//			rc = btc_node_io_send(io, rmsg, MG_MQTT_QOS(qos));
//			rc = redis_sup_by_topic(ioctx->c, io->sid, rmsg->topic, rmsg->mid, 0);
//
//			if(hp_log_level > 0){
//				hp_log(stdout, "%s: Redis sup, fd=%d, io='%s', key/value='%s'/'%s'\n", __FUNCTION__
//						, ((hp_io_t *)io)->fd, io->sid, rmsg->topic, rmsg->mid);
//			}
//			sdsclear(rmsg->mid);
//		}
//	}
//
//	/* fetch next message to send */
//	listNode * node = 0;
//
//	if(!io->l_msg)
//		io->l_msg = listFirst(io->outlist);
//	else{
//		node = io->l_msg;
//		io->l_msg = listNextNode(io->l_msg);
//	}
//
//	if(node)
//		listDelNode(io->outlist, node);
//
//	if(!io->l_msg)
//		goto ret;	/* empty message, nothing to send */
//
//	io->l_time = 0;
//	io->l_mid = 0;
//ret:
	return rc;
}

int btc_node_ctx_init(btc_node_io_ctx * ioctx
	, hp_sock_t fd, int tcp_keepalive
	, int ping_interval)
{
	int rc;
	if (!(ioctx)) { return -1; }

	hp_ioopt ioopt = { fd, 0, s_btc_nodehdl
#ifdef _MSC_VER
		, 200  /* poll timeout */
		, 0    /* hwnd */
#endif /* _MSC_VER */
	};
	rc = hp_io_init((hp_io_ctx *)ioctx, &ioopt);
	if (rc != 0) { return -3; }

	ioctx->rping_interval = ping_interval;

	return rc;
}

int btc_node_ctx_run(btc_node_io_ctx * ioctx)
{
	return hp_io_run((hp_io_ctx *)ioctx, 200, 0);
}

int btc_node_ctx_uninit(btc_node_io_ctx * ioctx)
{
	return hp_io_uninit((hp_io_ctx *)ioctx);
}

/////////////////////////////////////////////////////////////////////////////////////////
