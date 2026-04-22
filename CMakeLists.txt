###########################################################################################
#
# This file is PART of btcc project
#
# author hongjun.liao <docici@126.com>
# date 2023/6/27

###########################################################################################
cmake_minimum_required(VERSION 2.8.12)
include(deps/libhp/cmakes/libhp.cmake) # hp_cmake_find_deps

###########################################################################################
# btcc
project(btcc)
#set(CMAKE_CXX_STANDARD 20)
###########################################################################################
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Options are: None, Debug, Release, RelWithDebInfo, MinSizeRel." FORCE)
endif()
# -DCMAKE_EXPORT_COMPILE_COMMANDS is for lsp/clangd, see
# https://clangd.llvm.org/installation.html
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

##########################################################################################
# find_program(CLANG NAMES clang)
# if (CLANG)
#     set(CMAKE_C_COMPILER "${CLANG}")
#     set(CMAKE_CXX_COMPILER "${CLANG}")
# 	message("${CL}")
# endif() 

###########################################################################################
# sources and deps
if(WIN32)
	# getopt
	set(getopt_lib "deps/getopt/*.c")
	set(db_inc deps/db/build_windows)
else()
	set(db_inc "deps/")
	set(getopt_lib c)
endif()

set(g_withs 1 1 1 1 1 1 1 1 1) 
set(g_hdrs sqlite3.h .nullfilesub.h .nullfilesub.h .nullfilesrc.h getopt.h openssl/ssl.h db_cxx.h .nullfilesrc.h boost/foreach.hpp)
set(g_incs sqlite3 "secp256k1/include/" "deps/libhp/include/" "${PROJECT_BINARY_DIR}/ deps/libhp/deps/" deps/getopt/ deps/openssl/include ${db_inc} deps/ deps/boost_1_82_0)
set(g_deps sqlite3 libhp secp256k1 .src getopt openssl db cryptopp boost_1_82_0)
set(g_libs sqlite3 libhp secp256k1 "src/*.cpp src/test/*.cpp"  ${getopt_lib} "ssl crypto"  db_cxx "deps/cryptopp/*.cpp" "boost_filesystem boost_unit_test_framework")
###########################################################################################
# for libhp
set(LIBHP_WITH_HTTP 1 CACHE INTERNAL "LIBHP_WITH_HTTP")

hp_cmake_find_deps(SRCS BITCOIN_WITH_ deps cmakes g_withs g_hdrs g_incs g_deps g_libs)
#message("btcc: SRCS=${SRCS}")
add_executable(btcc ${SRCS})

###########################################################################################

foreach(dep IN LISTS g_deps  )
	message("btcc: dep='${dep}', ${dep}_INCLUDE_DIRS='${${dep}_INCLUDE_DIRS}', ${dep}_LIBRARIES='${${dep}_LIBRARIES}'")
	target_include_directories(btcc PRIVATE ${${dep}_INCLUDE_DIRS} )
	target_link_libraries(btcc ${${dep}_LIBRARIES} ) 
endforeach() 

target_compile_options(btcc PUBLIC
	-DHAVE_CONFIG_H
    -DCMAKE_EXPORT_COMPILE_COMMANDS
    # for redis
    -DBOOST_TEST_DYN_LINK
)

###########################################################################################
configure_file("config.h.in" "config.h")

