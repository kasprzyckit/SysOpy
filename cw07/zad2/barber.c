#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include "posix.h"

int fifo = 0;
barber_state* bs;
sem_t* shop_state;
sem_t* cust_present;
sem_t* barb_ready;
sem_t* cust_ready;
sem_t* cut_done;
sem_t* seat_occupied;

void sigexit(int sig) {exit(EXIT_FAILURE);}

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
    int i;
    for(i = 0; i<MAX_CLIENTS; i++) sem_destroy(&(bs->prv[i]));
    munmap(bs, sizeof(barber_state));
    shm_unlink(BARBER_SHARED);
    sem_close(shop_state);
    sem_close(cust_present);
    sem_close(barb_ready);
    sem_close(cust_ready);
    sem_close(cut_done);
    sem_close(seat_occupied);
    sem_unlink(SEM_SHOP);
    sem_unlink(SEM_CPRES);
    sem_unlink(SEM_BREAD);
    sem_unlink(SEM_CREAD);
    sem_unlink(SEM_CUT);
    sem_unlink(SEM_SEAT);
}

void init_resources(void)
{
    if (mkfifo(FIFO_PATH, S_IRWXU | S_IRWXG | S_IRWXO) < 0) 
    {
        if (errno == EEXIST) errno = 0;
        else err("Barber make pipe");
    }

    int ss, i;

    if ((fifo = open(FIFO_PATH, O_RDONLY | O_NONBLOCK)) < 0) err("Barber pipe");
    if ((ss = shm_open(BARBER_SHARED, O_RDWR | O_CREAT, \
        S_IRWXU | S_IRWXG | S_IRWXO)) < 0) err("Shared segment");
    if (ftruncate(ss, sizeof(barber_state)) < 0) err("Ftruncate");
    if ((bs = (barber_state*) mmap(NULL, sizeof(barber_state), \
        PROT_READ | PROT_WRITE, MAP_SHARED, ss, 0)) < 0) err("MMap");
    close(ss);
    bs->is_asleep = 0;
    bs->top = 0;
    for(i = 0; i<MAX_CLIENTS; i++) sem_init(&(bs->prv[i]), 1, 0);

    shop_state = sem_open(SEM_SHOP, O_CREAT, 0777, 1);
    cust_present = sem_open(SEM_CPRES, O_CREAT, 0777, 0);
    barb_ready = sem_open(SEM_BREAD, O_CREAT, 0777, 0);
    cust_ready = sem_open(SEM_CREAD, O_CREAT, 0777, 0);
    cut_done = sem_open(SEM_CUT, O_CREAT, 0777, 0);
    seat_occupied = sem_open(SEM_SEAT, O_CREAT, 0777, 1);
}

void print_msg(char* const msg, int id)
{
    char buff[100];
    struct timespec tp;
    sprintf(buff, "%s", msg);
    if (id != -1) sprintf(buff, "%s "ANSI_BLUE"%i"ANSI_RESET, buff, id);
    clock_gettime(CLOCK_MONOTONIC, &tp);
    sprintf(buff, "%s :: "ANSI_MAGNETA"%ld:%ld"ANSI_RESET"\n", buff, tp.tv_sec, tp.tv_nsec);
    printf("%s", buff);
    fflush(stdout);
}

void barber(int seats)
{
    msg_t msg;
    sem_wait(shop_state);
    if (bs->seats_free == seats)
    {
        print_msg("Barber goes to sleep\t\t", -1);
        bs->is_asleep = 1;
        sem_post(shop_state);
        sem_wait(cust_present);
        print_msg("Barber wakes up\t\t", -1);
    }
    else
    {
        read(fifo, &msg, sizeof(msg_t));
        bs->served_customer = msg.id;
        print_msg("Barber calls in\t\t", bs->served_customer);
        bs->seats_free += 1;
        sem_post(&(bs->prv[msg.sem]));
        sem_post(shop_state);
    }
    sem_post(barb_ready);
    sem_wait(cust_ready);
    print_msg("Barber starts cutting\t", bs->served_customer);
    print_msg("Barber ends cutting\t", bs->served_customer);
    sem_post(cut_done);
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