#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sched.h>
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

void err(const char* msg);
void __exit(void);
void init_resources(void);
void print_msg(char* const msg);
int get_sem(void);

void client(int cuts)
{
    printf("Client "ANSI_BLUE"%i"ANSI_RESET" init\n", getpid());
    int prvsem = get_sem(), count = 0;
    msg_t msg;
    msg.id = getpid();
    msg.sem = prvsem;
    for (; cuts > 0;)
    {
        sem_wait(shop_state);
        if (bs->is_asleep)
        {
        	count = 0;
            print_msg("Client wakes the barber\t\t");
            bs->is_asleep = 0;
            bs->served_customer = getpid();
            sem_post(cust_present);
            sem_post(shop_state);
        	sem_wait(seat_occupied);        //2
        }
        else
        {
            if (bs->seats_free)
            {
            	count = 0;
                print_msg("Client enters the queue\t\t");
                write(fifo, &msg, sizeof(msg_t));
                bs->seats_free -= 1;
                sem_post(shop_state);
                sem_wait(&(bs->prv[prvsem]));
        		sem_wait(seat_occupied);        //2
                sem_wait(shop_state);       //1
                bs->seats_free += 1;        //1
                sem_post(shop_state);       //1
            }
            else
            {
                print_msg("Client leaves without a haircut\t");
                sem_post(shop_state);
            //    if (count > 50) sched_yield();
            //    count++;
                continue;
            }
        }
        sem_wait(barb_ready);
        print_msg("Client takes the seat\t\t");
        sem_post(cust_ready);
        sem_wait(cut_done);
        print_msg("Client leaves with a haircut\t");
        sem_post(seat_occupied);        //2
        cuts--;
    }

    printf("Client "ANSI_BLUE"%i"ANSI_RESET" exit\n", getpid());
    _exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[])
{
    if (argc < 3) err("Too few arguments!");
    int clients = atoi(argv[1]);
    int cuts = atoi(argv[2]);
    if (clients <= 0 || cuts  <= 0) err("Invalid argumnts!");

    atexit(__exit);
    signal(SIGTERM, sigexit);
    signal(SIGSEGV, sigexit);
    signal(SIGINT, sigexit);
    init_resources();

    int i;
    pid_t tmp;

    for (i = 0; i < clients; i++)
    {
        tmp = fork();
        if (tmp < 0) err("Fork");
        if (tmp == 0) client(cuts);
    }

    exit(EXIT_SUCCESS);
}

void print_msg(char* const msg)
{
    char buff[100];
    struct timespec tp;
    sprintf(buff, "%s "ANSI_BLUE"%i"ANSI_RESET, msg, getpid());
    clock_gettime(CLOCK_MONOTONIC, &tp);
    sprintf(buff, "%s :: "ANSI_MAGNETA"%ld:%ld" \
        ANSI_RESET"\n", buff, tp.tv_sec, tp.tv_nsec);
    printf("%s", buff);
    fflush(stdout);
}

int get_sem(void)
{
    if (bs->top == MAX_CLIENTS-1) bs->top = 0;
    else bs->top++;
    return bs->top;
}

void init_resources(void)
{
    int ss;
    
    if ((fifo = open(FIFO_PATH, O_WRONLY)) < 0) err("Client pipe");
    if ((ss = shm_open(BARBER_SHARED, O_RDWR, 0)) < 0) err("Shared segment");
    if ((bs = (barber_state*) mmap(NULL, sizeof(barber_state), \
        PROT_READ | PROT_WRITE, MAP_SHARED, ss, 0)) < 0) err("MMap");
    close(ss);

    shop_state = sem_open(SEM_SHOP, 0);
    cust_present = sem_open(SEM_CPRES, 0);
    barb_ready = sem_open(SEM_BREAD, 0);
    cust_ready = sem_open(SEM_CREAD, 0);
    cut_done = sem_open(SEM_CUT, 0);
    seat_occupied = sem_open(SEM_SEAT, 0);
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
    munmap(bs, sizeof(barber_state));
    sem_close(shop_state);
    sem_close(cust_present);
    sem_close(barb_ready);
    sem_close(cust_ready);
    sem_close(cut_done);
    sem_close(seat_occupied);
}
