cmake_minimum_required(VERSION 3.10)

project(winsup C)

set(SOURCES
    str.c
)

add_library(winsup ${SOURCES})
target_link_options(winsup PUBLIC -Wall -Wextra -Wformat -O2)

set(WINSUP_INCLUDE
    str_compat.h
    PARENT_SCOPE
)