/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef BTC_LOG__H
#define BTC_LOG__H
#include "btc_protocol.h"
#include <hp/sdsinc.h>

int btc_log_p2p( btc_p2p_hdr const * hdr, btc_p2p_payload const * payload, int flags);

#endif //BTC_LOG__H
