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
#include "hp/string_util.h"
#include "hp/hp_log.h"
#include "hp/hp_assert.h"
#include "hp/hp_net.h"
#include "hp/hp_http.h"
#include "hp/hp_config.h"

#include "btc_net.h"
#include "btc_node.h"

/////////////////////////////////////////////////////////////////////////////////////////////
extern hp_ini * g_ini;
#define cfg(k) hp_config_ini(g_ini, (k))
#define cfgi(k) atoi(cfg(k))

#define return_(code) do{ rc = code; goto exit_; } while(0)
int test_btc_main(int argc, char ** argv);
/////////////////////////////////////////////////////////////////////////////////////////////
class CChainParams {
public:
    std::vector<std::string> vSeeds;
};
class CMainParams: public CChainParams {
	CMainParams(){
        vSeeds.emplace_back("seed.bitcoin.sipa.be."); // Pieter Wuille, only supports x1, x5, x9, and xd
        vSeeds.emplace_back("dnsseed.bluematt.me."); // Matt Corallo, only supports x9
        vSeeds.emplace_back("dnsseed.bitcoin.dashjr-list-of-p2p-nodes.us."); // Luke Dashjr
        vSeeds.emplace_back("seed.bitcoin.jonasschnelli.ch."); // Jonas Schnelli, only supports x1, x5, x9, and xd
        vSeeds.emplace_back("seed.btc.petertodd.net."); // Peter Todd, only supports x1, x5, x9, and xd
        vSeeds.emplace_back("seed.bitcoin.sprovoost.nl."); // Sjors Provoost
        vSeeds.emplace_back("dnsseed.emzy.de."); // Stephan Oeste
        vSeeds.emplace_back("seed.bitcoin.wiz.biz."); // Jason Maurice
        vSeeds.emplace_back("seed.mainnet.achownodes.xyz."); // Ava Chow, only supports x1, x5, x9, x49, x809, x849, xd, x400, x404, x408, x448, xc08, xc48, x40c
	}
};

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
		hp_log(std::cerr, "%s: unable to listen on %d for BTC node\n", __FUNCTION__, cfgi("btc.port"));
		return_(-7);
	}
	http_listenfd = hp_tcp_listen(cfgi("http.port"));
	if(!hp_sock_is_valid(btc_listenfd)){
		hp_log(std::cerr, "%s: unable to listen on %d for HTTP\n", __FUNCTION__, cfgi("http.port"));
		return_(-8);
	}

	if(hp_io_init(ioctx, opt) != 0) return_(-11);
	if(hp_http_init(http, ioctx, http_listenfd, 0, btc_http_process) != 0) return_(-12);
	if(btc_init(bctx, ioctx, btc_listenfd, 0, 0) != 0) return_(-13);

	/* run */
	hp_log(std::cout, "%s: listening on BTC/HTTP port=%d/%d, waiting for connection ...\n", __FUNCTION__
			, cfgi("btc.port"), cfgi("http.port"));

	btc_connect(bctx, "8333");
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
	hp_log(rc == 0? std::cout : std::cerr, "%s: exited with %d\n", __FUNCTION__, rc);
#endif //#ifndef NDEBUG

	return rc;
}

bool init_function() {
    return true;
}

int main(int argc, char ** argv)
{
	boost::unit_test::unit_test_main(init_function, argc, argv);
	return btc_main(argc, argv);
}
/////////////////////////////////////////////////////////////////////////////////////////////
