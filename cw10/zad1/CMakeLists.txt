SET(SRC calc.c)

add_executable(ServerStream server.c ${SRC})
add_executable(ClientStream client.c ${SRC})