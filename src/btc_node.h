/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef BTC_NODE_H
#define BTC_NODE_H

#include "hp/hp_io_t.h"
#ifdef __cplusplus
extern "C" {
#include "redis/src/adlist.h"	  /* list */
#endif
#ifdef __cplusplus
}
#endif

typedef struct  {
	hp_io_ctx  * ioctx;
	hp_io_t 	listenio;
	int rping_interval; /* redis ping-pong interval */
	uint16_t mqid;		/* mqtt message_id */
	list * inlist;		/* node coming */
	list * outlist;		/* nodes sent out */

} btc_node_ctx;

typedef struct {
	hp_io_t io;
	btc_node_ctx * bctx;
} btc_node;



/////////////////////////////////////////////////////////////////////////////////////////

int btc_node_init(btc_node * node, btc_node_ctx * ioctx);
void btc_node_uninit(btc_node * node);
/////////////////////////////////////////////////////////////////////////////////////////

int btc_init(btc_node_ctx * bctx
		, hp_io_ctx * ioctx, hp_iohdl hdl
		, hp_sock_t fd, int tcp_keepalive
		, int ping_interval);
void btc_uninit(btc_node_ctx * ioctx);

/////////////////////////////////////////////////////////////////////////////////////////
/**
 *  for nodes  out
 */
btc_node * btc_out_find(btc_node_ctx * bctx, void * key, int (* match)(void *ptr, void *key));
#define btc_out_count(bctx) (listLength(bctx->outlist))

/////////////////////////////////////////////////////////////////////////////////////////

/**
 * for nodes in
 */
#define btc_in_count(bctx) (listLength(bctx->inlist))
/////////////////////////////////////////////////////////////////////////////////////////


#endif
