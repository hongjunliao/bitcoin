/*! 
 * mastering taproot 
 */
 #include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h> //TestingSetup

 struct SignetSetup : public TestingSetup {
	SignetSetup()
        : TestingSetup{ChainType::SIGNET} {}
};
 BOOST_FIXTURE_TEST_SUITE(mastering_taproot, SignetSetup)

 BOOST_AUTO_TEST_CASE(pri_key)
{
    CKey extkey;
    BOOST_CHECK(!extkey.IsValid());
    extkey.MakeNewKey(true);
    BOOST_CHECK(extkey.IsValid());
}

BOOST_AUTO_TEST_SUITE_END()