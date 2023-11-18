cmake_minimum_required(VERSION 3.26)
project(src C)

set(CMAKE_C_STANDARD 11)

add_executable(src main.c
        globals.c
        globals.h
        server.c
        server.h
        hash_table.c
        hash_table.h
        disk.c
        disk.h)
