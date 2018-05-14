#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include "systemv.h"

int fifo = 0, semid, semprv, ss;
barber_state* bs;

void sigexit(int sig) {exit(EXIT_FAILURE);}
void err(const char* msg);
void __exit(void);
void init_resources(void);
void print_msg(char* const msg, int id);

void barber(int seats)
{
    msg_t msg;
    struct sembuf sems[1];
    sems[0].sem_flg = 0;
    SEMWAIT(semid, SEM_SHOP)
    if (bs->seats_free == seats)
    {
        print_msg("Barber goes to sleep\t\t", -1);
        bs->is_asleep = 1;
        SEMPOST(semid, SEM_SHOP)
        SEMWAIT(semid, SEM_CPRES)
        print_msg("Barber wakes up\t\t", -1);
    }
    else
    {
        read(fifo, &msg, sizeof(msg_t));
        bs->served_customer = msg.id;
        print_msg("Barber calls in\t\t", bs->served_customer);
        SEMPOST(semprv, msg.sem)
        SEMPOST(semid, SEM_SHOP)
    }
    SEMPOST(semid, SEM_BREAD)
    SEMWAIT(semid, SEM_CREAD)
    print_msg("Barber starts cutting\t", bs->served_customer);
    print_msg("Barber ends cutting\t", bs->served_customer);
    SEMPOST(semid, SEM_CUT)
    SEMWAIT(semid, SEM_CREAD)
}

int main(int argc, char const *argv[])
{
    if (argc < 2) err("Too few arguments!");
    int seats = atoi(argv[1]);
    if (seats <= 0) err("Invalid argumnt!");

    atexit(__exit);
    signal(SIGTERM, sigexit);
    signal(SIGINT, sigexit);
    signal(SIGSEGV, sigexit);
    init_resources();
    bs->seats_free = seats;

    printf("Barber init\n");

    while (1) barber(seats);


    exit(EXIT_FAILURE);
}

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf(ANSI_RED"%s\n"ANSI_RESET, msg);
    exit(EXIT_FAILURE);
}

void __exit(void)
{
    if (fifo) close(fifo);
    unlink(FIFO_PATH);
    shmdt(bs);
    shmctl(ss, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, NULL);
    semctl(semprv, 0, IPC_RMID, NULL);
}

void init_resources(void)
{
    if (mkfifo(FIFO_PATH, S_IRWXU | S_IRWXG | S_IRWXO) < 0) 
    {
        if (errno == EEXIST) errno = 0;
        else err("Barber make pipe");
    }

    if ((fifo = open(FIFO_PATH, O_RDONLY | O_NONBLOCK)) < 0) err("Barber pipe");
    
    if ((ss = shmget(ftok(SPATH, 0), sizeof(barber_state), \
        IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO)) < 0) err("Shmget");
    if ((bs = (barber_state*)shmat(ss, NULL, 0)) < 0) err("Shmat");
    bs->is_asleep = 0;
    bs->top = 0;

    if ((semid = semget(ftok(SPATH, 1), 7, \
        IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO)) < 0) err("Semget");
    if ((semprv = semget(ftok(SPATH, 2), MAX_CLIENTS, \
        IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO)) < 0) err("Semget");
    union semun arg;
    int i;
    arg.val = 1;
    semctl(semid, SEM_SHOP, SETVAL, arg);
    semctl(semid, SEM_SEAT, SETVAL, arg);
    semctl(semid, QUE_SEAT, SETVAL, arg);
    arg.val = 0;
    semctl(semid, SEM_CPRES, SETVAL, arg);
    semctl(semid, SEM_BREAD, SETVAL, arg);
    semctl(semid, SEM_CREAD, SETVAL, arg);
    semctl(semid, SEM_CUT, SETVAL, arg);
    for (i = 0; i < MAX_CLIENTS; i++) semctl(semprv, i, SETVAL, arg);
}

void print_msg(char* const msg, int id)
{
    char buff[100];
    struct timespec tp;
    sprintf(buff, "%s", msg);
    if (id != -1) sprintf(buff, "%s "ANSI_BLUE"%i"ANSI_RESET, buff, id);
    clock_gettime(CLOCK_MONOTONIC, &tp);
    sprintf(buff, "%s :: "ANSI_MAGNETA"%ld:%ld" \
        ANSI_RESET"\n", buff, tp.tv_sec, tp.tv_nsec);
    printf("%s", buff);
    fflush(stdout);
}