SET(SRC calc.c)

add_executable(ServerDatagram server.c ${SRC})
add_executable(ClientDatagram client.c ${SRC})