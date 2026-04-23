/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
#include <boost/test/unit_test.hpp>
#include "btc_log.h"
#include "btc_test.h"
#include "btc_protocol.h"
#include <iostream>
#include <hp/hp_log.h>

int btc_log_p2p( btc_p2p_hdr const * hdr, btc_p2p_payload const * payload, int flags)
{
	if(!(hdr)) return -1;

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
			"(payload)");
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
