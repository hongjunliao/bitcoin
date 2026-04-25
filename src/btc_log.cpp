/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
#include "btc_log.h"
#include <boost/test/unit_test.hpp>
#include <time.h>
#include "btc_inc.h"
#include "btc_protocol.h"
#include <iostream>

int btc_log_p2p( btc_p2p_hdr const * hdr, btc_p2p_payload const * pl, int flags)
{
	if(!(hdr && pl)) return -1;

	sds plbuf = sdsnew(hdr->command);
	if(strncmpl(hdr->command, "ping") == 0) {
		plbuf = sdscatprintf(plbuf, ":%u", (uint32_t)pl->pong.c);
	}
	else if(strncmpl(hdr->command, "version") == 0) {
		char tbuf[128] = "";
		strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&pl->version.timestamp));

		plbuf = sdscatprintf(plbuf, ":timestamp=%s;agent=%s", tbuf, pl->version.user_agent);
	}

	hp_log(stdout, "%s%s\n"
					   "\tmagic:    %X%X%X%X\n"
					   "\tcommand:  %s\n"
					   "\tsize:     %u\n"
					   "\tchecksum: %X%X%X%X\n"
					   "\tpayload:  %s\n",
			flags? "->": "<-",
			hdr->command,
			hdr->magic[0],hdr->magic[1],hdr->magic[2],hdr->magic[3],
			hdr->command,
			hdr->length,
			hdr->checksum[0],hdr->checksum[1],hdr->checksum[2],hdr->checksum[3],
			plbuf);

	sdsfree(plbuf);
	return 0;
}


BOOST_AUTO_TEST_SUITE()
BOOST_AUTO_TEST_CASE(_01)
{
	btc_p2p_hdr hdrobj, * hdr = &hdrobj; btc_p2p_payload payloadobj, *payload = &payloadobj;
    memcpy(hdrobj.magic, "\xf9\xbe\xb4\xd9", 4); // Mainnet magic
    strcpy(hdrobj.command, "version");
	hdr->length = 2;
    btc_log_p2p(hdr, payload, 0);
}
BOOST_AUTO_TEST_SUITE_END()
