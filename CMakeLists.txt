# author hongjun.liao <docici@126.com>
# date 2023/6/27
cmake_minimum_required(VERSION 3.0)

configure_file("config.h.in" "config.h")

if(NOT MSVC)
find_package(PkgConfig REQUIRED)
pkg_check_modules(openssl REQUIRED openssl)
find_path(db_cxx db_cxx.h PATHS /usr/include /usr/local/include)
	if(NOT db_cxx)
		message(WARNING "db_cxx.h NOT found, using deps/db_cxx instead")
		add_subdirectory(deps/db_cxx) 
	endif()
else()
# Windows specific
endif()

add_subdirectory(deps/libhp) 

# bitcoin-v0.1.5 
project(bitcoin-v0.1.5)

file(GLOB SRCS src/*.cpp src/*.c deps/sds/* deps/cryptopp/*)
add_executable(bitcoin-v0.1.5 ${SRCS})

target_include_directories(bitcoin-v0.1.5 PRIVATE 
	"${PROJECT_BINARY_DIR}" 
	${openssl_INCLUDE_DIRS} ${db_INCLUDE_DIRS}____________db_INCLUDE_DIRS
	deps/libhp/include deps/)
target_compile_options(bitcoin-v0.1.5 PUBLIC -D_GNU_SOURCE -DCMAKE_EXPORT_COMPILE_COMMANDS)
target_link_libraries(bitcoin-v0.1.5 ${openssl_LIBRARIES} ${db_LIBRARIES} libhp db_cxx)
