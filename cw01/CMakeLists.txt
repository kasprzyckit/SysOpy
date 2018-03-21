cmake_minimum_required(VERSION 2.8)
project(CharBlocks C)

set(CMAKE_C_STANDARD 99)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(SOURCE_FILES ../zad1/char_blocks.c)
set(HEADER_FILES ../zad1/char_blocks.h)
set(MAIN_FILE ../zad2/main.c)

set(ARGS DYNAMIC 10000 create_table 1000 10 add 250 search_element asdfghjklm)

add_subdirectory (zad1)
add_subdirectory (zad2)
add_subdirectory (zad3a)
add_subdirectory (zad3b)


add_custom_target(All DEPENDS CharBlocksTest CharBlocksLibraries CharBlocksOptimization)