###########################################################################################
#
# This file is PART of bitcoin-v0.1.5 project
#
# author hongjun.liao <docici@126.com>
# date 2023/6/27

###########################################################################################
cmake_minimum_required(VERSION 2.8.12)
include(deps/libhp/cmakefiles/libhp.cmake) # hp_cmake_find_deps

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
set(g_withs 1 1 1 1 1 1 1 1)
set(g_hdrs openssl/ssl.h db_cxx.h .nullfilesub.h .nullfilesrc.h .nullfilesrc.h .nullfilesrc.h 
	uv.h .nullfilesrc.h)
set(g_incs .find_path .find_path "deps/libhp/include/" .find_path "${PROJECT_BINARY_DIR}/ deps/" .find_path 
	.find_path .find_path)
set(g_deps openssl db_cxx libhp sds .src cryptopp 
	uv inih)

set(g_libs "ssl crypto"  db_cxx libhp "deps/sds/*.c" "src/*.cpp " "deps/cryptopp/*.cpp" 
	uv deps/inih/*.c)

###########################################################################################

# for libhp
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(LIBHP_WITH_SSL 1 CACHE INTERNAL "LIBHP_WITH_SSL")
	set(LIBHP_WITH_ZLIB 1 CACHE INTERNAL "LIBHP_WITH_ZLIB")
	set(LIBHP_WITH_BDB 1 CACHE INTERNAL "LIBHP_WITH_BDB")
	set(LIBHP_WITH_CURL 1 CACHE INTERNAL "LIBHP_WITH_CURL")
	set(LIBHP_WITH_REDIS 1 CACHE INTERNAL "LIBHP_WITH_REDIS")
	set(LIBHP_WITH_MYSQL 1 CACHE INTERNAL "LIBHP_WITH_MYSQL")
	set(LIBHP_WITH_HTTP 1 CACHE INTERNAL "LIBHP_WITH_HTTP")
	set(LIBHP_WITH_MQTT 1 CACHE INTERNAL "LIBHP_WITH_MQTT")
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

hp_cmake_find_deps(SRCS LIBHP_WITH_ deps g_withs g_hdrs g_incs g_deps g_libs)
#message("bitcoin-v0.1.5: SRCS=${SRCS}")
add_executable(bitcoin-v0.1.5 ${SRCS})

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
if(MSVC)
	target_compile_options(bitcoin-v0.1.5 PUBLIC -D_CRT_SECURE_NO_WARNINGS /MTd)
endif()

###########################################################################################
configure_file("config.h.in" "config.h")

