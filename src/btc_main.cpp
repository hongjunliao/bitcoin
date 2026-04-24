/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
//
#include "config.h"
#include <getopt.h>		/* getopt_long */
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "hp/sdsinc.h"	//sds
#include "hp/hp_str.h"
#include "hp/hp_log.h"
#include "hp/hp_assert.h"
#include "hp/hp_net.h"
#include "hp/hp_http.h"
#include "hp/hp_ini.h"

#include "btc_net.h"
#include "btc_node.h"
#include "btc_protocol.h"
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

/////////////////////////////////////////////////////////////////////////////////////////
int btc_inih_handler(void* user, const char* section, const char* name,const char* value);
static hp_ini iniobj = {.parser = btc_inih_handler };
//global default configure
hp_ini * g_ini = &iniobj;
/////////////////////////////////////////////////////////////////////////////////////////////
extern hp_ini * g_ini;
#define cfg(k) hp_ini_exec(g_ini, (k))
#define cfgi(k) atoi(cfg(k))
#define cfgv(...) hp_ini_execv(g_ini, __VA_ARGS__)

#define return_(code) do{ rc = code; goto exit_; } while(0)

static int btc_node_send(btc_node * outnode, sds buf)
{
	return hp_io_write(&outnode->io, buf, sdslen(buf), [](void * p){ assert(p); sdsfree((sds)p); }, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////
static int btc_http_process(struct hp_http * http, hp_httpreq * req, struct hp_httpresp * resp)
{
	assert(http && req && resp);

	resp->status_code = 200;
	resp->flags = 0;
	resp->html = req->url;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
static int btc_connect(btc_node_ctx *bctx);
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
	rc = btc_node_send(node, btc_p2pmsg_reply(hdr, pl, &outhdr, &outpl));
	assert(rc == 0);
	btc_log_p2p(&outhdr, &outpl, 1);

//	char const * ack = hdr->command;
//	if(strncmpl(hdr->command, "version") == 0) 	 		ack = "verack";
//	else if(strncmpl(hdr->command, "ping") == 0)	 	ack = "pong";
//	else if(strncmpl(hdr->command, "verack") == 0) 		ack = "verack";
//	else if(strncmpl(hdr->command, "wtxidrelay") == 0) 	ack = "";
//	else if(strncmpl(hdr->command, "sendaddrv2") == 0) 	ack = "";
//
//	if(strlen(ack) > 0){
//	}

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

static int btc_connect(btc_node_ctx *bctx)
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
	rc = btc_node_send(outnode, btc_p2pmsg_reply(0, 0, &outhdr, &outpl));
	assert(rc == 0);

	btc_log_p2p(&outhdr, &outpl, 1);
//	hp_log(stdout, "%s: sent, message=version, payload_len=%d\n", __FUNCTION__, 0/*header.length*/);
	listAddNodeTail(bctx->outlist, outnode);

//	rc = hp_io_write(&outnode->io, message, BTC_HDR_SIZE + header.length, [](void * p){ assert(p); delete (uint8_t *)p; }, 0);
//	outnode->io.addr = addr;
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////


int btc_main(int argc, char ** argv)
{
	int rc = 0;
	hp_ioopt opt = {
		.timeout = 0
#ifdef _MSC_VER
	.wm_user = 900, /* WM_USER + N */
	.hwnd = 0		/* hwnd */
#endif /* _MSC_VER */
	};
	/* HTTP server */
	hp_http httpobj, * http = &httpobj;
	/* BTC node server */
	btc_node_ctx bctxobj = { 0 }, * bctx = &bctxobj;
	/* IO context */
	hp_io_ctx ioctxobj, * ioctx = &ioctxobj;
	/* listen fd */
	hp_sock_t btc_listenfd, http_listenfd;
	time_t lastt = 0;
	fprintf(stdout, "%s: build at %s %s\n", __FUNCTION__, __DATE__, __TIME__);
   /////////////////////////////////////////////////////////////////////////////////////////////
	srandom(time(NULL));
    /* parse argc/argv */
	// load default configure if exits
	cfg("#load bitcoin.conf");
	// global log level
	hp_log_level  = cfgi("loglevel");
	/////////////////////////////////////////////////////////////////////////////////////////////
	btc_listenfd = hp_tcp_listen(cfgi("btc.port"));
	if(!hp_sock_is_valid(btc_listenfd)){
		hp_log(stderr, "%s: unable to listen on %d for BTC node\n", __FUNCTION__, cfgi("btc.port"));
		return_(-7);
	}
	http_listenfd = hp_tcp_listen(cfgi("http.port"));
	if(!hp_sock_is_valid(btc_listenfd)){
		hp_log(stderr, "%s: unable to listen on %d for HTTP\n", __FUNCTION__, cfgi("http.port"));
		return_(-8);
	}

	if(hp_io_init(ioctx, opt) != 0) return_(-11);
	if(hp_http_init(http, ioctx, http_listenfd, 0, btc_http_process) != 0) return_(-12);
	if(btc_init(bctx, ioctx, s_btc_in_node_hdl, btc_listenfd, 0, 0) != 0) return_(-13);

	/* run */
	hp_log(stdout, "%s: listening on BTC/HTTP port=%d/%d, waiting for connection ...\n", __FUNCTION__
			, cfgi("btc.port"), cfgi("http.port"));

	btc_connect(bctx);
	for (;;) {
		hp_io_run(ioctx, 1);
	}

	//clean
	btc_uninit(bctx);
	hp_http_uninit(http);
	hp_io_uninit(ioctx);
	hp_close(http_listenfd);
	hp_close(btc_listenfd);
	cfg("#unload");
exit_:
#ifndef NDEBUG
	hp_log(rc == 0? stdout : stderr, "%s: exited with %d\n", __FUNCTION__, rc);
#endif //#ifndef NDEBUG

	return rc;
}
/////////////////////////////////////////////////////////////////////////////////////////////
