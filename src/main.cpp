/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
//
#include "config.h"
int btc_parse_args(int argc, char ** argv);

#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/unit_test.hpp>
#include <hp/sdsinc.h>
int btc_main(int argc, char ** argv);

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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#else
#define BOOST_TEST_DISABLE_ALT_STACK
#define BOOST_TEST_MODULE btcc

#include <boost/test/included/unit_test.hpp>

#include <test/util/setup_common.h>

#include <functional>
#include <iostream>

/**
 * Retrieve the command line arguments from boost.
 * Allows usage like:
 * `test_bitcoin --run_test="net_tests/cnode_listen_port" -- -checkaddrman=1 -printtoconsole=1`
 * which would return `["-checkaddrman=1", "-printtoconsole=1"]`.
 */
const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS = []() {
	std::vector<const char*> args;
	for (int i = 1; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
		args.push_back(boost::unit_test::framework::master_test_suite().argv[i]);
	}
	return args;
};

/**
 * Retrieve the boost unit test name.
 */
const std::function<std::string()> G_TEST_GET_FULL_NAME = []() {
	return boost::unit_test::framework::current_test_case().full_name();
};

/////////////////////////////////////////////////////////////////////////////////////////////
struct ArgvHandler {
	void setup() {
		auto argc = boost::unit_test::framework::master_test_suite().argc;
		auto argv = boost::unit_test::framework::master_test_suite().argv;
		if (btc_parse_args(argc, argv) != 0) return ;
	}
	void teardown() {}
};

BOOST_TEST_GLOBAL_FIXTURE(ArgvHandler);
#endif
/////////////////////////////////////////////////////////////////////////////////////////////