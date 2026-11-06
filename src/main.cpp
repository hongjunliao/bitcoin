/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
//
#include "config.h"
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/unit_test.hpp>
#include <hp/sdsinc.h>
int btc_main(int argc, char ** argv);
int btc_parse_args(int argc, char ** argv);

static bool init_function() {
	auto argc = boost::unit_test::framework::master_test_suite().argc;
	auto argv = boost::unit_test::framework::master_test_suite().argv;
	if (btc_parse_args(argc, argv) != 0) return 0;
    return true;
}

int main(int argc, char ** argv)
{
	boost::unit_test::unit_test_main(init_function, argc, argv);
	return btc_main(argc, argv);
}
/////////////////////////////////////////////////////////////////////////////////////////////
