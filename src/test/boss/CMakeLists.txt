add_executable(test_boss
        main.cpp
        mastering_taptroot.cpp
        balance.cpp
        spend.cpp
)

target_link_libraries(test_boss
    core_interface
    test_util
    bitcoin_cli
    bitcoin_node
    bitcoin_consensus
    minisketch
    secp256k1
    Boost::headers
    $<TARGET_NAME_IF_EXISTS:USDT::headers>
)

add_windows_application_manifest(test_boss)

add_test(NAME test_boss COMMAND test_boss)
