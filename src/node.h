/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef BTC_NODE_H
#define BTC_NODE_H

#include <functional>
#include <openssl/rand.h>
#include "hp/hp_io_t.h"
#include "hp/sdsinc.h"
#ifdef __cplusplus
extern "C" {
#include "redis/src/dict.h"	  /* dict */
#include "redis/src/adlist.h"	  /* list */
#include "hp/hp_http.h"
#endif
#ifdef __cplusplus
}
#endif
#include "net.h"

struct btc_node_ctx {
	hp_io_ctx  * ioctx;
	hp_io_t 	listenio;
	int rping_interval; /* redis ping-pong interval */
	uint16_t mqid;		/* mqtt message_id */
	list * inlist;		/* node coming */
	list * outlist;		/* nodes sent out */

};

class CNode {
public:
	hp_io_t io;
	btc_node_ctx * bctx;
};
typedef class CNode btc_node;



/////////////////////////////////////////////////////////////////////////////////////////

int btc_node_init(btc_node * node, btc_node_ctx * ioctx);
void btc_node_uninit(btc_node * node);
/////////////////////////////////////////////////////////////////////////////////////////

int btc_init(btc_node_ctx * bctx
		, hp_io_ctx * ioctx
		, hp_sock_t fd, int tcp_keepalive
		, int ping_interval);
int btc_connect(btc_node_ctx *bctx, char const * port);
void btc_uninit(btc_node_ctx * ioctx);

/////////////////////////////////////////////////////////////////////////////////////////
/**
 *  for nodes  out
 */
btc_node * btc_out_find(btc_node_ctx * bctx, void * key, int (* match)(void *ptr, void *key));
#define btc_out_count(bctx) (listLength(bctx->outlist))

/////////////////////////////////////////////////////////////////////////////////////////

int btc_send(btc_node * outnode, char const * hdr_
		, std::function<void(CDataStream& ds)>  const& datacb);
/////////////////////////////////////////////////////////////////////////////////////////

/**
 * for nodes in
 */
#define btc_in_count(bctx) (listLength(bctx->inlist))
/////////////////////////////////////////////////////////////////////////////////////////

int btc_http_process(struct hp_http * http, hp_httpreq * req, struct hp_httpresp * resp);

/////////////////////////////////////////////////////////////////////////////////////////////

#endif
