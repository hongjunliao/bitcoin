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
#include "hp/hp_io_t.h"      /* ,... */
#include "hp/hp_log.h"
#include "hp/str_dump.h"
#include "btc_node.h"

#include "hp/hp_http.h"
#include "hp/hp_net.h"
#include "hp/hp_config.h"
extern "C"{
#include "redis/src/dict.h"	  	/* dict */
#include "redis/src/adlist.h"	/* list */
}
#include "btc_protocol.h"

extern hp_ini * g_ini;
#define cfg(k) hp_config_ini(g_ini, (k))
#define cfgi(k) atoi(cfg(k))
/////////////////////////////////////////////////////////////////////////////////////////
#define return_(code) do{ rc = code; goto exit_; } while(0)
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
		hp_log(std::cout, "%s: New BTC connection from '%s', total=%d\n", __FUNCTION__
				, hp_addr4name(&cio->addr, ":", buf, sizeof(buf)), btc_in_count(bctx));
	}

//	int64 nTime = (true/*fInbound*/ ? GetAdjustedTime() : GetTime());
//	rc  = btc_send(innode, "version"
//			, [&](CDataStream& ds){ ds << VERSION << 0 << nTime << "localhost"; });
	assert((rc == 0));

	return (hp_io_t *)innode;
}

static int btc_node_on_parse(hp_io_t * io, char * buf, size_t * len
	, void ** hdrp, void ** bodyp)
 {
	assert(io && hdrp && bodyp);
	int rc = 0;
	auto innode = (btc_node*) io;
	if(!(*len >= BTC_HDR_SIZE)) { return(0); }

	// Read header
	auto phdr = new MessageHeader;
	memcpy(phdr, buf, BTC_HDR_SIZE);

	if(!(memcmp(phdr->magic, "\xf9\xbe\xb4\xd9", 4) == 0)){
		*len = 0;
		return_(-2);
	}
	if(*len - BTC_HDR_SIZE < phdr->length){
		return_(0);
	}

	*len -= (BTC_HDR_SIZE + phdr->length);
	if(strncmp(phdr->command, "version", 7) == 0){
		*bodyp = sdsnewlen(buf + BTC_HDR_SIZE, phdr->length);
	}
	else if(strncmp(phdr->command, "ping", 4) == 0){
		*bodyp = sdsnewlen(buf + BTC_HDR_SIZE, phdr->length);
	}
	else{
		hp_log(std::cout, "%s: NOT support message=%s, payload_len=%d\n", __FUNCTION__, phdr->command, phdr->length);
	}

	*hdrp = phdr;
	rc = 1;
exit_:
	if(rc <= 0) { delete phdr; }
	return rc;
}

static int btc_node_on_dispatch(hp_io_t * io, void * hdr, void * body)
{
	assert(io && hdr);
	int rc;
	auto node = (btc_node*) io;
	auto phdr = (MessageHeader *)hdr;

	if(strncmp(phdr->command, "version", 7) == 0){
		auto payload = (sds)body;
		assert(phdr->length == sdslen(payload));
		strcpy(phdr->command, "verack");

	    // Serialize version message
	    uint8_t * message = new uint8_t[BTC_HDR_SIZE+phdr->length];
	    memcpy(message, phdr, BTC_HDR_SIZE);
	    memcpy(message + BTC_HDR_SIZE, payload, phdr->length);

		rc = hp_io_write(&node->io, message, BTC_HDR_SIZE + phdr->length
					, [](void * p){ assert(p); delete (uint8_t *)p; }, 0);
		assert(rc == 0);
		hp_log(std::cout, "%s: sent, message=verack, payload_len=%d\n", __FUNCTION__, phdr->length);

		sdsfree(payload);
	}
	else if(strncmp(phdr->command, "ping", 4) == 0){

	}
	delete phdr;

//	CDataStream &vRecv = innode->vRecv;
//	MessageHeader &btchdr = *(MessageHeader *)hdr;
//
//	CDataStream vMsg(vRecv.begin(), vRecv.begin() + btchdr.nMessageSize, vRecv.nType, vRecv.nVersion);
//	extern bool ProcessMessage(btc_node * pfrom, string strCommand, CDataStream& vRecv);
//	rc = ProcessMessage(innode, btchdr.GetCommand(), vMsg)? 0 : -1;
//
//	vRecv.ignore(btchdr.nMessageSize);
//	delete &btchdr;

	return rc;
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
		hp_log(std::cout, "%s: Delete BTC connection '%s', %d/'%s', total=%d\n", __FUNCTION__
				, hp_addr4name(&io->addr, ":", buf, sizeof(buf)), err, errstr, btc_in_count(bctx));
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
		hp_log(std::cout, "%s: Disconnected from '%s', %d/'%s', total=%d\n", __FUNCTION__
				, hp_addr4name(&io->addr, ":", buf, sizeof(buf)), err, errstr, btc_in_count(bctx));
	}
	listDelNode(bctx->outlist, node);
	btc_node_uninit(outnode);
	delete (outnode);

	if(listLength(bctx->inlist) < cfgi("peer.count"))
		btc_connect(bctx, "8333");
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

int btc_connect(btc_node_ctx *bctx, char const * port)
{
	int rc;
	if(!(bctx)) return -1;
    std::vector<std::string> vSeeds;
////	vSeeds.emplace_back("seed.bitcoin.sipa.be."); // Pieter Wuille, only supports x1, x5, x9, and xd
//	vSeeds.emplace_back("dnsseed.bluematt.me."); // Matt Corallo, only supports x9
//	vSeeds.emplace_back("dnsseed.bitcoin.dashjr-list-of-p2p-nodes.us."); // Luke Dashjr
////	vSeeds.emplace_back("seed.bitcoin.jonasschnelli.ch."); // Jonas Schnelli, only supports x1, x5, x9, and xd
//	vSeeds.emplace_back("seed.btc.petertodd.net."); // Peter Todd, only supports x1, x5, x9, and xd
//	vSeeds.emplace_back("seed.bitcoin.sprovoost.nl."); // Sjors Provoost
//	vSeeds.emplace_back("dnsseed.emzy.de."); // Stephan Oeste
//	vSeeds.emplace_back("seed.bitcoin.wiz.biz."); // Jason Maurice
//	vSeeds.emplace_back("seed.mainnet.achownodes.xyz."); // Ava Chow, only supports x1, x5, x9, x49, x809, x849, xd, x400, x404, x408, x448, xc08, xc48, x40c
    vSeeds.emplace_back("127.0.0.1");
	struct addrinfo addr;
	auto & seed = vSeeds[random() % vSeeds.size()];
	hp_sock_t confd = hp_connect(seed.c_str(), port, &addr);
	if (!hp_sock_is_valid(confd)) {
		return -2;
	}
	char addrstr[128] = "";
	hp_log(std::cout, "%s: connected '%s/%s'\n", __FUNCTION__, seed, hp_ntop(&addr, ":", addrstr, sizeof(addrstr)));

	auto outnode = new btc_node;
	assert(outnode);

	rc = btc_node_init(outnode, bctx);
	assert(rc == 0);
	rc = hp_io_add(bctx->ioctx, (hp_io_t*) outnode, confd, s_btc_out_node_hdl);
	assert(rc == 0);

	/////////////////////////////////////////////////////////////////////////////////////////

    CVersionMsg version_msg = {0};
    version_msg.version = 70016; // Protocol version
    version_msg.services = 1ULL | (1ULL << 10); // NODE_NETWORK | NODE_WITNESS
    version_msg.timestamp = time(NULL);

    // Serialize version message
    uint8_t * message = new uint8_t[512];
    size_t payload_len = serialize_version_msg(&version_msg, message + BTC_HDR_SIZE, 512 - BTC_HDR_SIZE);
    if (payload_len == 0) {
    	delete message;
    	delete outnode;
        return -3;
    }
//    pchMessageStart[0] = 0xf9;
    MessageHeader header = {0}; assert(sizeof(MessageHeader) == BTC_HDR_SIZE);
    memcpy(header.magic, "\xf9\xbe\xb4\xd9", 4); // Mainnet magic
    strcpy(header.command, "version");
    header.length = payload_len;
    compute_checksum(message + BTC_HDR_SIZE, payload_len, header.checksum);

    // Combine header and payload
    memcpy(message, &header, BTC_HDR_SIZE);

	rc = hp_io_write(&outnode->io, message, BTC_HDR_SIZE + header.length
				, [](void * p){ assert(p); delete (uint8_t *)p; }, 0);
	assert(rc == 0);
	hp_log(std::cout, "%s: sent, message=version, payload_len=%d\n", __FUNCTION__, header.length);

//	outnode->io.addr = addr;
	listAddNodeTail(bctx->outlist, outnode);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
