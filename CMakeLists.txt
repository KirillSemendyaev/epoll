cmake_minimum_required(VERSION 3.5)
project(epoll)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES epoll_server.c udp_client.c tcp_client.c)
add_executable(UDP_CLIENT udp_client.c)
add_executable(TCP_CLIENT tcp_client.c)
add_executable(EPOLL_SERVER epoll_server.c)