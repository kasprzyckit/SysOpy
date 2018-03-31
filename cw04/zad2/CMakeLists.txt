add_executable(SIGUSR sigusr.c)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -lrt")