add_executable(Master master.c)
add_executable(Slave slave.c)

add_custom_target(Main
    DEPENDS Master Slave
    COMMAND ./master-slave.sh fifo)