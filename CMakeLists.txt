cmake_minimum_required(VERSION 3.19)
project(OS_HW5 C)

set(CMAKE_C_STANDARD 99)

add_executable(OS_HW5 main.c pcc_client..c pcc_server.c)