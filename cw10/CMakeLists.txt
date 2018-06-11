cmake_minimum_required(VERSION 2.8)
project(Sockets C)

set(CMAKE_C_STANDARD 99)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
link_libraries(pthread)

add_subdirectory (zad1)
add_subdirectory (zad2)