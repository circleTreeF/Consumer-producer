cmake_minimum_required(VERSION 3.17)
project(cw C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-std=c99 -pthread")
include_directories(.)
add_executable(cw
        coursework.c linkedlist.c coursework.h linkedlist.h
        #        TASK1a.c
        # TASK1bDebug.c
        #task1bFprintf.c
        TASK2aDebug.c)