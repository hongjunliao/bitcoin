/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
//
#include "config.h"
#include <boost/test/unit_test.hpp>
int btc_main(int argc, char ** argv);

static bool init_function() {
    return true;
}

int main(int argc, char ** argv)
{
	boost::unit_test::unit_test_main(init_function, argc, argv);
	return -1;
	return btc_main(argc, argv);
}
/////////////////////////////////////////////////////////////////////////////////////////////
