###########################################################################################
#
# This file is PART of bitcoin-v0.1.5 project
#
# author hongjun.liao <docici@126.com>
# date 2023/6/27

###########################################################################################
cmake_minimum_required(VERSION 2.8.12)
include(deps/libhp/cmakes/libhp.cmake) # hp_cmake_find_deps

###########################################################################################
# bitcoin-v0.1.5 
project(bitcoin-v0.1.5)

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

set(g_withs 1 1 1 1 1 1 1)
set(g_hdrs .nullfilesub.h .nullfilesrc.h getopt.h openssl/ssl.h db_cxx.h .nullfilesrc.h boost/foreach.hpp)
set(g_incs "deps/libhp/include/" "${PROJECT_BINARY_DIR}/ deps/libhp/deps/" deps/getopt/ deps/openssl/include ${db_inc} deps/ deps/boost_1_82_0)
set(g_deps libhp .src getopt openssl db cryptopp boost_1_82_0)
set(g_libs libhp "src/*.cpp"  ${getopt_lib} "ssl crypto"  db_cxx "deps/cryptopp/*.cpp" c)
###########################################################################################

# for libhp
set(LIBHP_WITH_HTTP 1 CACHE INTERNAL "LIBHP_WITH_HTTP")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(LIBHP_WITH_SSL 1 CACHE INTERNAL "LIBHP_WITH_SSL")
	set(LIBHP_WITH_ZLIB 1 CACHE INTERNAL "LIBHP_WITH_ZLIB")
	set(LIBHP_WITH_BDB 1 CACHE INTERNAL "LIBHP_WITH_BDB")
	set(LIBHP_WITH_CURL 1 CACHE INTERNAL "LIBHP_WITH_CURL")
#	set(LIBHP_WITH_REDIS 1 CACHE INTERNAL "LIBHP_WITH_REDIS")
#	set(LIBHP_WITH_MYSQL 1 CACHE INTERNAL "LIBHP_WITH_MYSQL")
#	set(LIBHP_WITH_MQTT 1 CACHE INTERNAL "LIBHP_WITH_MQTT")
	set(LIBHP_WITH_PROC_SYSINFO 1 CACHE INTERNAL "LIBHP_WITH_PROC_SYSINFO")
	set(LIBHP_WITH_AMQP 1 CACHE INTERNAL "LIBHP_WITH_AMQP")
#	set(LIBHP_WITH_ZLOG 1 CACHE INTERNAL "LIBHP_WITH_ZLOG")
	set(LIBHP_WITH_TIMERFD 1 CACHE INTERNAL "LIBHP_WITH_TIMERFD")
	set(LIBHP_WITH_JNI 1 CACHE INTERNAL "LIBHP_WITH_JNI")
	set(LIBHP_WITH_CJSON 1 CACHE INTERNAL "LIBHP_WITH_CJSON")
	set(LIBHP_WITH_OPTPARSE 1 CACHE INTERNAL "LIBHP_WITH_OPTPARSE")
	set(LIBHP_WITH_DLFCN 1 CACHE INTERNAL "LIBHP_WITH_DLFCN")
#	set(LIBHP_DEPRECADTED 1 CACHE INTERNAL "LIBHP_DEPRECADTED")
endif()

hp_cmake_find_deps(SRCS LIBHP_WITH_ deps cmakes g_withs g_hdrs g_incs g_deps g_libs)
#message("bitcoin-v0.1.5: SRCS=${SRCS}")

add_executable(bitcoin-v0.1.5 ${SRCS})

# FIXME
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
add_compile_options(/W0)
	set_property(TARGET bitcoin-v0.1.5 PROPERTY COMPILE_OPTIONS "-w")
#	set_property(SOURCE src/main.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -Wall")	
#	set_property(SOURCE test/test_btc.cpp PRIVATE COMPILE_OPTIONS "-Wall")
endif()
###########################################################################################

foreach(dep IN LISTS g_deps  )
	message("bitcoin-v0.1.5: dep='${dep}', ${dep}_INCLUDE_DIRS='${${dep}_INCLUDE_DIRS}', ${dep}_LIBRARIES='${${dep}_LIBRARIES}'")
	target_include_directories(bitcoin-v0.1.5 PRIVATE ${${dep}_INCLUDE_DIRS} )
	target_link_libraries(bitcoin-v0.1.5 ${${dep}_LIBRARIES} ) 
endforeach() 

target_compile_options(bitcoin-v0.1.5 PUBLIC
	-DHAVE_CONFIG_H
    -DCMAKE_EXPORT_COMPILE_COMMANDS
    # for redis
    -DDICT_BENCHMARK_MAIN
	# for mg
	-DMG_ENABLE_DIRECTORY_LISTING
    # for cJSON
    -DcJSON_FREE_PRINT
)

###########################################################################################
configure_file("config.h.in" "config.h")

