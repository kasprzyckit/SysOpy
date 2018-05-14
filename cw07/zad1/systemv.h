#ifndef __SYSV
#define __SYSV

#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RED     "\x1b[91m"
#define ANSI_MAGNETA "\x1b[35m"
#define ANSI_RESET   "\x1b[0m"

#define MAX_CLIENTS 100

typedef struct
{
    short is_asleep;
    int seats_free;
    pid_t served_customer;
    int top;
} barber_state;

union semun
{
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
    struct seminfo  *__buf;
};

typedef struct
{
    pid_t id;
    int sem;
} msg_t;

#define FIFO_PATH "fifo"
#define SPATH getenv("HOME")
#define SEM_SHOP    0
#define SEM_CPRES   1
#define SEM_BREAD   2
#define SEM_CREAD   3
#define SEM_CUT     4
#define SEM_SEAT    5
#define QUE_SEAT    6

#define SEMWAIT(semid, sem) sems[0].sem_num = sem; \
    sems[0].sem_op = -1; \
    semop(semid, sems, 1);

#define SEMPOST(semid, sem) sems[0].sem_num = sem; \
    sems[0].sem_op = 1; \
    semop(semid, sems, 1);

#define SEMWAIT2(semid, sem1, sem2) sems[0].sem_num = sem1; \
        sems[0].sem_op = -1; \
        sems[1].sem_num = sem2; \
        sems[1].sem_op = -1; \
        semop(semid, sems, 2);

#define SEMPOST2(semid, sem1, sem2) sems[0].sem_num = sem1; \
        sems[0].sem_op = 1; \
        sems[1].sem_num = sem2; \
        sems[1].sem_op = 1; \
        semop(semid, sems, 2);

#endif