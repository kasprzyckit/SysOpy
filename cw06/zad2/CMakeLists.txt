add_executable(ServerP server.c)
add_executable(ClientP client.c)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lrt")
