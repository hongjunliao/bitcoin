/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>     /* assert */
#include <arpa/inet.h>
#include "hp/hp_io_t.h"      /* ,... */
#include "hp/hp_log.h"
#include "hp/hp_str.h"
#include "btc_node.h"
#include "btc_net.h"	//
#include "btc_log.h"
#include "btc_inc.h"
#include "hp/hp_http.h"
#include "hp/hp_net.h"
#include "hp/hp_ini.h"
extern "C"{
#include "redis/src/dict.h"	  	/* dict */
#include "redis/src/adlist.h"	/* list */
}
#include "btc_protocol.h"

extern hp_ini * g_ini;
#define cfg(k) hp_ini_exec(g_ini, (k))
#define cfgi(k) atoi(cfg(k))
#define cfgv(...) hp_ini_execv(g_ini, __VA_ARGS__)
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

static hp_io_t *  btc_node_in_on_new(hp_io_t * cio, hp_sock_t fd)
{
	assert(cio && cio->ioctx && cio->user);

	btc_node_ctx * bctx = (btc_node_ctx *)cio->user;

	auto innode = new btc_node;
	int rc = btc_node_init(innode, bctx);
	assert(rc == 0);

	/* nio is NOT a listen io */
	hp_iohdl niohdl = cio->iohdl;
	niohdl.on_new = 0;
	rc = hp_io_add(cio->ioctx, (hp_io_t *)innode, fd, niohdl);
	if (rc != 0) {
		btc_node_uninit(innode);
		delete innode;
		return 0;
	}

	innode->io.addr = cio->addr;
	listAddNodeTail(bctx->inlist, innode);

	if(hp_log_level > 0){
		char buf[64] = "";
		hp_log(stdout, "%s: New BTC connection from '%s', total=%d\n", __FUNCTION__
				, hp_addr4name(&cio->addr, ":", buf, sizeof(buf)), btc_in_count(bctx));
	}

//	int64 nTime = (true/*fInbound*/ ? GetAdjustedTime() : GetTime());
//	rc  = btc_send(innode, "version"
//			, [&](CDataStream& ds){ ds << VERSION << 0 << nTime << "localhost"; });
	assert((rc == 0));

	return (hp_io_t *)innode;
}

static int btc_node_in_on_loop(hp_io_t * io)
{
	assert(io);
	int rc = 0;
	if(io->iohdl.on_new)
		return 0;

	auto node = (btc_node *)io;
//	bool SendMessages(CNode* pto);
//	SendMessages(node);

	return rc;
}

static void btc_node_in_on_delete(hp_io_t * io, int err, char const * errstr)
{
	btc_node * innode = (btc_node *)io;
	assert(innode && innode->bctx);
	btc_node_ctx * bctx = innode->bctx;

	listNode * node = listSearchKey(innode->bctx->inlist, io);
	assert(node);

	if(hp_log_level > 0){
		char buf[64] = "";
		hp_log(stdout, "%s: Delete BTC connection '%s', %d/'%s', total=%d\n", __FUNCTION__
				, hp_addr4name(&io->addr, ":", buf, sizeof(buf)), err, errstr, btc_in_count(bctx));
	}

	listDelNode(bctx->inlist, node);

	btc_node_uninit(innode);
	delete(innode);
}

static int btc_node_on_parse(hp_io_t * io, char * buf, size_t * len, void ** hdrp, void ** bodyp)
{
	assert(io && hdrp && bodyp);
	auto innode = (btc_node*) io;

	auto hdr = new btc_p2p_hdr;
	auto pl = new btc_p2p_payload;
	int rc = 0;
	if(btc_p2pmsg_parse(buf, *len, hdr, pl) == 0){
		rc = 1;
		//剩余的部分是下一个消息的数据(如果不为0的话)
		*len -= (BTC_HDR_SIZE + hdr->length);
		*hdrp = hdr;
		*bodyp = pl;
	}
	else { delete hdr; delete pl; }
	return rc;
}

static int btc_node_on_dispatch(hp_io_t * io, void * hdrp, void * bodyp)
{
	assert(io && hdrp);
	int rc;
	auto node = (btc_node*) io;
	auto hdr = (btc_p2p_hdr *)hdrp;
	auto pl = (btc_p2p_payload *)bodyp;
	btc_log_p2p(hdr, pl, 0);

	btc_p2p_hdr outhdr{0}; btc_p2p_payload outpl{0};
	char const * ack = hdr->command;
	if(strncmpl(hdr->command, "version") == 0) 	 		ack = "verack";
	else if(strncmpl(hdr->command, "ping") == 0)	 	ack = "pong";
	else if(strncmpl(hdr->command, "verack") == 0) 		ack = "verack";
	else if(strncmpl(hdr->command, "wtxidrelay") == 0) 	ack = "";
	else if(strncmpl(hdr->command, "sendaddrv2") == 0) 	ack = "";

	if(strlen(ack) > 0){
		rc = btc_node_send(node, btc_p2pmsg_new(hdr, pl, &outhdr, &outpl));
		assert(rc == 0);
		btc_log_p2p(&outhdr, &outpl, 1);
	}

	delete (pl);
	delete hdr;

	return rc;
}

/* callbacks for btc_node in */
static hp_iohdl s_btc_in_node_hdl = {
	.on_new = btc_node_in_on_new,
	.on_parse = btc_node_on_parse,
	.on_dispatch = btc_node_on_dispatch,
	.on_loop = btc_node_in_on_loop,
	.on_delete = btc_node_in_on_delete
};

/////////////////////////////////////////////////////////////////////////////////////////

static void btc_node_out_on_delete(hp_io_t * io, int err, char const * errstr)
{
	btc_node * outnode = (btc_node *)io;
	assert(outnode && outnode->bctx);
	btc_node_ctx * bctx = outnode->bctx;

	listNode * node = listSearchKey(outnode->bctx->outlist, outnode);
	assert(node);

	if(hp_log_level > 0){
		char buf[64] = "";
		hp_log(stdout, "%s: Disconnected from '%s', %d/'%s', total=%d\n", __FUNCTION__
				, hp_addr4name(&io->addr, ":", buf, sizeof(buf)), err, errstr, btc_in_count(bctx));
	}
	listDelNode(bctx->outlist, node);
	btc_node_uninit(outnode);
	delete (outnode);

	if(listLength(bctx->inlist) < cfgi("peer.count"))
		btc_connect(bctx);
}

static int btc_node_out_on_loop(hp_io_t * io)
{
	return 0;
}

/* callbacks for btc_node out */
static hp_iohdl s_btc_out_node_hdl = {
	.on_new = 0,
	.on_parse = btc_node_on_parse,
	.on_dispatch = btc_node_on_dispatch,
	.on_loop = btc_node_out_on_loop,
	.on_delete = btc_node_out_on_delete
};

/////////////////////////////////////////////////////////////////////////////////////////

int btc_init(btc_node_ctx * bctx
	, hp_io_ctx * ioctx
	, hp_sock_t fd, int tcp_keepalive
	, int ping_interval)
{
	int rc;
	if (!(bctx && ioctx)) { return -1; }

	bctx->ioctx = ioctx;
	bctx->rping_interval = ping_interval;

	rc = hp_io_add(bctx->ioctx, &bctx->listenio, fd, s_btc_in_node_hdl);
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

int btc_node_send(btc_node * outnode, sds buf)
{
	return hp_io_write(&outnode->io, buf, sdslen(buf), [](void * p){ assert(p); sdsfree((sds)p); }, 0);

}
int btc_connect(btc_node_ctx *bctx)
{
	int rc;
	if(!(bctx)) return -1;

	char ip_str[INET6_ADDRSTRLEN + 64]; // 足够存储 IPv4 或 IPv6 地址
	if(cfg("btc.p2p")[0] == ':'){
		do{
				auto p = btc_rand_p2p();
				// 根据地址族（IPv4 或 IPv6）提取 IP 地址
				if (p.ai_family == AF_INET) { // IPv4
					struct sockaddr_in *ipv4 = (struct sockaddr_in *)p.ai_addr;
					auto ret = inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, sizeof(ip_str));
					if(ret && strncmp(ip_str, "0.", 2) != 0 && strncmp(ip_str, "::", 2) != 0) break;
				} else if (p.ai_family == AF_INET6) { // IPv6
					struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p.ai_addr;
					inet_ntop(AF_INET6, &(ipv6->sin6_addr), ip_str, sizeof(ip_str));
					continue;
				} else {
					continue;
//					return -1; // 忽略不支持的地址族
				}
				cfgv("#set btc.p2p %s%s", ip_str, cfg("btc.p2p"));
		}while(1);
	}
	hp_log(stdout, "%s: connecting to '%s' ...\n", __FUNCTION__, cfg("btc.p2p"));
	hp_sock_t confd = hp_tcp_connect(cfg("btc.p2p"));
	if (!hp_sock_is_valid(confd)) {
		return -2;
	}
	hp_log(stdout, "%s: connected '%s'\n", __FUNCTION__, cfg("btc.p2p"));

	auto outnode = new btc_node;
	assert(outnode);

	rc = btc_node_init(outnode, bctx);
	assert(rc == 0);
	rc = hp_io_add(bctx->ioctx, (hp_io_t*) outnode, confd, s_btc_out_node_hdl);
	assert(rc == 0);

	btc_p2p_hdr outhdr; btc_p2p_payload outpl;
	rc = btc_node_send(outnode, btc_p2pmsg_newc("version", 0, &outhdr, &outpl));
	assert(rc == 0);

	btc_log_p2p(&outhdr, &outpl, 1);
//	hp_log(stdout, "%s: sent, message=version, payload_len=%d\n", __FUNCTION__, 0/*header.length*/);
	listAddNodeTail(bctx->outlist, outnode);

//	rc = hp_io_write(&outnode->io, message, BTC_HDR_SIZE + header.length, [](void * p){ assert(p); delete (uint8_t *)p; }, 0);
//	outnode->io.addr = addr;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
