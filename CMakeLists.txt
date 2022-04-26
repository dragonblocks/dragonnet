cmake_minimum_required(VERSION 3.14)
project(Dragonnet)

add_compile_options(
	-Wall
	-Wextra
	-Werror
)

add_library(dragonnet
	dragonnet/addr.c
	dragonnet/error.c
	dragonnet/init.c
	dragonnet/listen.c
	dragonnet/peer.c
	dragonnet/recv.c
	dragonnet/recv_thread.c
	dragonnet/send.c
)

target_link_libraries(dragonnet
	pthread
)

target_include_directories(dragonnet
	PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
	PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/endian.h"
)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	target_link_libraries(dragonnet
		ws2_32
	)
endif()
