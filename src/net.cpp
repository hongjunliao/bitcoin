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

int btc_out_send(btc_node * outnode, btc_msg * msg, hp_io_free_t freecb)
{
	int rc;
	if(!(outnode && msg))
		return -1;
	rc = hp_io_write((hp_io_t *)outnode, &msg->ds[0], msg->ds.size(), freecb, 0);

#ifndef NDEBUG
	hp_log(std::cout, "%s: sent message: '%s'/%d\n", __FUNCTION__, "", msg->ds.size());
#endif

	return rc;
}

int btc_node_init(btc_node * node, btc_node_ctx * bctx)
{
	if(!(node && bctx))
		return -1;

	memset(node, 0, sizeof(btc_node));
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
	btc_node * innode = (btc_node *)calloc(1, sizeof(btc_node));
	int rc = btc_node_init(innode, bctx);
	assert(rc == 0);

	listAddNodeTail(bctx->inlist, innode);

	if(hp_log_level > 0){
		char buf[64] = "";
		hp_log(std::cout, "%s: new BTC connection from '%s', total=%d\n", __FUNCTION__
				, get_ipport_cstr2(&cio->addr, ":", buf, sizeof(buf)), btc_in_count(bctx));
	}
	return (hp_io_t *)innode;
}

static int btc_node_on_parse(hp_io_t * io, char * buf, size_t * len
	, void ** hdrp, void ** bodyp)
 {
	assert(io && hdrp);
	int rc;
#define quit_(code) do{ rc = code; goto exit_; }while(0)
	auto innode = (btc_node*) io;
	// Read header
	auto phdr = new CMessageHeader;
	CMessageHeader &hdr = *phdr;
	*hdrp = phdr;

	CDataStream &vRecv = innode->inds;
	vRecv.insert(vRecv.end(), buf, buf + * len);

	std::vector<CAddress> addr;
	int n = END(pchMessageStart) - BEGIN(pchMessageStart);
	// Scan for message start
	CDataStream::iterator pstart = search(vRecv.begin(), vRecv.end(),
			BEGIN(pchMessageStart), END(pchMessageStart));
	if (vRecv.end() - pstart < sizeof(CMessageHeader)) {
		if (vRecv.size() > sizeof(CMessageHeader)) {
			printf("\n\nPROCESSMESSAGE MESSAGESTART NOT FOUND\n\n");
			vRecv.erase(vRecv.begin(), vRecv.end() - sizeof(CMessageHeader));
		}
		quit_(0);
	}
	if (pstart - vRecv.begin() > 0)
		printf("\n\nPROCESSMESSAGE SKIPPED %d BYTES\n\n",
				pstart - vRecv.begin());
	vRecv.erase(vRecv.begin(), pstart);

	vRecv >> hdr;
	if (!hdr.IsValid()) {
		printf("\n\nPROCESSMESSAGE: ERRORS IN HEADER %s\n\n\n",
				hdr.GetCommand().c_str());
		*len = 0;
		quit_(-1);
	}
//    string strCommand = hdr.GetCommand();

// Message size
	if (hdr.nMessageSize > vRecv.size()) {
		// Rewind and wait for rest of message
		///// need a mechanism to give up waiting for overlong message size error
		printf("MESSAGE-BREAK\n");
		vRecv.insert(vRecv.begin(), BEGIN(hdr), END(hdr));
		quit_(0);
	}

	// Copy message to its own buffer
//    CDataStream vMsg(vRecv.begin(), vRecv.begin() + hdr.nMessageSize, vRecv.nType, vRecv.nVersion);
	vRecv.ignore(hdr.nMessageSize);
	vRecv >> addr;

	// Process message
//    bool fRet = false;
//    try
//    {
//        CheckForShutdown(2);
//        CRITICAL_BLOCK(cs_main)
//            fRet = ProcessMessage(pfrom, strCommand, vMsg);
//        CheckForShutdown(2);
//    }
//    CATCH_PRINT_EXCEPTION("ProcessMessage()")
//    if (!fRet)
//        printf("ProcessMessage(%s, %d bytes) from %s to %s FAILED\n", strCommand.c_str(), hdr.nMessageSize, pfrom->addr.ToString().c_str(), addrLocalHost.ToString().c_str());
	*len = 0;
exit_:
	return 1;
}

extern bool ProcessMessage(btc_node * pfrom, string strCommand, CDataStream& vRecv);
static int btc_node_on_dispatch(hp_io_t * io, void * hdr, void * body)
{
	assert(io && hdr);
	int rc;
	auto innode = (btc_node*) io;
	CDataStream &vRecv = innode->inds;
	CMessageHeader &btchdr = *(CMessageHeader *)hdr;
	std::vector<CAddress> addr;

	vRecv >> addr;
	CDataStream vMsg(vRecv.begin(), vRecv.begin() + btchdr.nMessageSize, vRecv.nType, vRecv.nVersion);

	vMsg >> addr;

	return ProcessMessage(innode, btchdr.GetCommand(), vMsg)? 0 : -1;
}

static int btc_node_on_loop(hp_io_t * io)
{
	if(io->iohdl.on_new)
		return 0;
	return btc_node_loop((btc_node *)io);
}

static void btc_node_on_delete(hp_io_t * io)
{
	btc_node * innode = (btc_node *)io;
	assert(innode && innode->bctx);
	btc_node_ctx * bctx = innode->bctx;

	listNode * node = listSearchKey(innode->bctx->inlist, io);
	assert(node);
	listDelNode(innode->bctx->inlist, node);

	if(hp_log_level > 0){
		char buf[64] = "";
		hp_log(std::cout, "%s: delete BTC connection '%s', total=%d\n", __FUNCTION__
				, get_ipport_cstr2(&io->addr, ":", buf, sizeof(buf)), btc_in_count(bctx));
	}

	listDelNode(bctx->inlist, node);

	btc_node_uninit(innode);
	free(innode);
}

/* callbacks for btc_node peers */
static hp_iohdl s_btc_peer_nodehdl = {
	.on_new = btc_node_on_new,
	.on_parse = btc_node_on_parse,
	.on_dispatch = btc_node_on_dispatch,
	.on_delete = btc_node_on_delete,
	.on_loop = btc_node_on_loop
};

/////////////////////////////////////////////////////////////////////////////////////////

static void btc_node_out_on_delete(hp_io_t * io)
{
	btc_node * outnode = (btc_node *)io;
	assert(outnode && outnode->bctx);
	btc_node_ctx * bctx = outnode->bctx;

	listNode * node = listSearchKey(outnode->bctx->outlist, outnode);
	assert(node);
	listDelNode(bctx->outlist, node);

	if(hp_log_level > 0){
		char buf[64] = "";
		hp_log(std::cout, "%s: disconnected from '%s', total=%d\n", __FUNCTION__
				, get_ipport_cstr2(&io->addr, ":", buf, sizeof(buf)), btc_in_count(bctx));
	}
	btc_node_uninit(outnode);
	free(outnode);
}

static int btc_node_out_on_loop(hp_io_t * io)
{
	return 0;
}

/* callbacks for btc_node sent */
static hp_iohdl s_btc_node_sent_hdl = {
	.on_new = 0,
	.on_parse = btc_node_on_parse,
	.on_dispatch = btc_node_on_dispatch,
	.on_delete = btc_node_out_on_delete,
	.on_loop = btc_node_out_on_loop
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

	rc = hp_io_add(bctx->ioctx, &bctx->listenio, fd, s_btc_peer_nodehdl);
	if (rc != 0) { return -4; }

	bctx->inlist = listCreate();
	bctx->outlist = listCreate();

	bctx->listenio.user = bctx;
	bctx->listenio.iohdl.on_loop = 0;

	return rc;
}

void btc_uninit(btc_node_ctx * bctx)
{
	listRelease(bctx->inlist);
	listRelease(bctx->outlist);
}

/////////////////////////////////////////////////////////////////////////////////////////

btc_node * btc_out_connect(btc_node_ctx * bctx, struct sockaddr_in addr)
{
	int rc;
	if(!(bctx)) return 0;

	hp_sock_t confd = hp_net_connect_addr2(addr);
	if (!hp_sock_is_valid(confd)) {
		return 0;
	}
	btc_node * outnode = (btc_node*) malloc(sizeof(btc_node));
	assert(outnode);

	rc = btc_node_init(outnode, bctx);
	assert(rc == 0);

	rc = hp_io_add(bctx->ioctx, (hp_io_t*) outnode, confd, s_btc_node_sent_hdl);
	if (rc != 0) {
		btc_node_uninit(outnode);
		free(outnode);
		return 0;
	}

	outnode->io.addr = addr;
	listAddNodeTail(bctx->outlist, outnode);
	return outnode;
}

btc_node * btc_out_find(btc_node_ctx * bctx, void * key, int (* match)(void *ptr, void *key))
{
	list li = { .match = bctx->outlist->match };
	listSetMatchMethod(bctx->outlist, match);
	listNode * node = listSearchKey(bctx->outlist, key);
	listSetMatchMethod(bctx->outlist, li.match);

	return (btc_node *)(node? listNodeValue(node) : 0);
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
