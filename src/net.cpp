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
extern CAddress addrLocalHost;
extern uint64 nLocalServices;
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

int btc_send(btc_node * outnode, char const * hdr_
		, std::function<void(CDataStream& ds)>  const& datacb)
{
	int rc = 0;

	btc_msg * msg = new btc_msg; assert(msg);
	(msg)->ds << CMessageHeader((hdr_), 0);
	if(datacb) { datacb((msg)->ds); }

	unsigned int nSize = (msg)->ds.size() - 0 - sizeof(CMessageHeader);
	memcpy((char*)&(msg)->ds[0] + offsetof(CMessageHeader, nMessageSize), &nSize, sizeof(nSize));

	rc = hp_io_write((hp_io_t *)outnode, &msg->ds[0], msg->ds.size(),
			[](void * msg){ assert(msg); delete (btc_msg *)msg; }, msg);

#ifndef NDEBUG
	hp_log(std::cout, "%s: sent message: %p/'%s'/%d\n", __FUNCTION__, msg, hdr_, msg->ds.size());
#endif

	if(!((rc) == 0)){ delete (msg); }

	return rc;
}
	
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
		hp_log(std::cout, "%s: New BTC connection from '%s', total=%d\n", __FUNCTION__
				, get_ipport_cstr2(&cio->addr, ":", buf, sizeof(buf)), btc_in_count(bctx));
	}

	int64 nTime = (true/*fInbound*/ ? GetAdjustedTime() : GetTime());
	rc  = btc_send(innode, "version"
			, [&](CDataStream& ds){ ds << VERSION << nLocalServices << nTime << addrLocalHost; });
	assert((rc == 0));

	return (hp_io_t *)innode;
}

static int btc_node_on_parse(hp_io_t * io, char * buf, size_t * len
	, void ** hdrp, void ** bodyp)
 {
#define return_(code) do{ rc = code; goto exit_; } while(0)
	assert(io && hdrp);
	int rc;
	auto innode = (btc_node*) io;

	if(!(*len >= sizeof(CMessageHeader))) { return(0); }

	// Read header
	auto phdr = new CMessageHeader;
	CMessageHeader &hdr = *phdr;

	CDataStream &vRecv = innode->inds;
	vRecv.insert(vRecv.end(), buf, buf + sizeof(CMessageHeader));

	int n = END(pchMessageStart) - BEGIN(pchMessageStart);
	// Scan for message start
	CDataStream::iterator pstart = search(vRecv.begin(), vRecv.end(),
			BEGIN(pchMessageStart), END(pchMessageStart));
	if (vRecv.end() - pstart < sizeof(CMessageHeader)) {
		printf("\n\nPROCESSMESSAGE MESSAGESTART NOT FOUND\n\n");
		vRecv.clear();

		*len -= sizeof(CMessageHeader);
		memmove(buf, buf + sizeof(CMessageHeader), *len);

		return_(0);
	}
	n = pstart - vRecv.begin();
	if (pstart - vRecv.begin() > 0)
		printf("\n\nPROCESSMESSAGE SKIPPED %d BYTES\n\n",
				pstart - vRecv.begin());
	vRecv.erase(vRecv.begin(), pstart);

	vRecv >> hdr;
	if (!hdr.IsValid()) {
		printf("\n\nPROCESSMESSAGE: ERRORS IN HEADER %s\n\n\n",
				hdr.GetCommand().c_str());

		*len -= sizeof(CMessageHeader);
		memmove(buf, buf + sizeof(CMessageHeader), *len);

		return_(0);
	}

// Message size
	if (hdr.nMessageSize > *len - sizeof(CMessageHeader)) {
		// Rewind and wait for rest of message
		///// need a mechanism to give up waiting for overlong message size error
		printf("MESSAGE-BREAK\n");

		vRecv.insert(vRecv.end(), buf + sizeof(CMessageHeader), buf + *len);
		*len = 0;

		return_(0);
	}
	//message body
	vRecv.insert(vRecv.end(), buf + sizeof(CMessageHeader), buf + sizeof(CMessageHeader) + hdr.nMessageSize );

	*hdrp = phdr;
	*len -= ( sizeof(CMessageHeader) + hdr.nMessageSize );
	memmove(buf, buf + sizeof(CMessageHeader) + hdr.nMessageSize, *len);

	rc = 1;
exit_:
	if(rc <= 0) { delete phdr; }
	return rc;
}

extern bool ProcessMessage(btc_node * pfrom, string strCommand, CDataStream& vRecv);
static int btc_node_on_dispatch(hp_io_t * io, void * hdr, void * body)
{
	assert(io && hdr);
	int rc;
	auto innode = (btc_node*) io;
	CDataStream &vRecv = innode->inds;
	CMessageHeader &btchdr = *(CMessageHeader *)hdr;

	CDataStream vMsg(vRecv.begin(), vRecv.begin() + btchdr.nMessageSize, vRecv.nType, vRecv.nVersion);
	rc = ProcessMessage(innode, btchdr.GetCommand(), vMsg)? 0 : -1;

	vRecv.ignore(btchdr.nMessageSize);
	delete &btchdr;

	return rc;
}

static int btc_node_in_on_loop(hp_io_t * io)
{
	if(io->iohdl.on_new)
		return 0;
	return btc_node_loop((btc_node *)io);
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
		hp_log(std::cout, "%s: Delete BTC connection '%s', %d/'%s', total=%d\n", __FUNCTION__
				, get_ipport_cstr2(&io->addr, ":", buf, sizeof(buf)), err, errstr, btc_in_count(bctx));
	}

	listDelNode(bctx->inlist, node);

	btc_node_uninit(innode);
	delete(innode);
}

/* callbacks for btc_node in */
static hp_iohdl s_btc_in_node_hdl = {
	.on_new = btc_node_in_on_new,
	.on_parse = btc_node_on_parse,
	.on_dispatch = btc_node_on_dispatch,
	.on_delete = btc_node_in_on_delete,
	.on_loop = btc_node_in_on_loop
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
		hp_log(std::cout, "%s: Disconnected from '%s', %d/'%s', total=%d\n", __FUNCTION__
				, get_ipport_cstr2(&io->addr, ":", buf, sizeof(buf)), err, errstr, btc_in_count(bctx));
	}
	listDelNode(bctx->outlist, node);
	btc_node_uninit(outnode);
	delete (outnode);
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

	rc = hp_io_add(bctx->ioctx, &bctx->listenio, fd, s_btc_in_node_hdl);
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
	auto outnode = new btc_node;
	assert(outnode);

	rc = btc_node_init(outnode, bctx);
	assert(rc == 0);

	rc = hp_io_add(bctx->ioctx, (hp_io_t*) outnode, confd, s_btc_out_node_hdl);
	assert(rc == 0);

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
