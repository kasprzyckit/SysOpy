#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int count1;
int count2;
pid_t child;
pid_t parent;
int mode;
sem_t* semaphore;

int sem_wait_nointr(sem_t *sem) {
  while (sem_wait(sem))
    if (errno == EINTR) errno = 0;
    else return -1;
  return 0;
}

void sig_parent(int sig)
{
    count2++;
    if (sig == SIGUSR1 || sig == SIGRTMIN)
    {
        printf(ANSI_COLOR_BLUE"Parent"ANSI_COLOR_RESET" process: received "ANSI_COLOR_MAGNETA \
            "%s"ANSI_COLOR_RESET" #%i\n", (sig == SIGUSR1) ? "SIGUSR1" : "SIGRTMIN", count2);
        if (mode == 2) sem_post(semaphore);
    }
    else if (sig == SIGINT)
    {
        printf(ANSI_COLOR_GREEN"\nParent"ANSI_COLOR_RESET" process: received "\
            ANSI_COLOR_RED"SIGINT\n"ANSI_COLOR_RESET);
        kill(child, (mode != 3) ? SIGUSR2 : SIGRTMAX);
        munmap(semaphore, sizeof(sem_t));
        exit(EXIT_SUCCESS);
    }
}

void sig_child(int sig)
{
    count1++;
    if (sig == SIGUSR1 || sig == SIGRTMIN)
    {
        printf(ANSI_COLOR_GREEN"Child"ANSI_COLOR_RESET" process: received "ANSI_COLOR_MAGNETA\
            "%s"ANSI_COLOR_RESET " #%i\n", (sig == SIGUSR1) ? "SIGUSR1" : "SIGRTMIN", count1);
        kill(parent, (sig == SIGUSR1) ? SIGUSR1 : SIGRTMIN);
    }
    else if (sig == SIGUSR2 || sig == SIGRTMAX)
    {
        printf(ANSI_COLOR_GREEN"Child"ANSI_COLOR_RESET" process: received "ANSI_COLOR_RED\
            "%s\n"ANSI_COLOR_RESET, (sig == SIGUSR2) ? "SIGUSR2" : "SIGRTMAX");
        sem_post(semaphore);
        exit(EXIT_SUCCESS);
    }
    if (mode == 1) sem_post(semaphore);
}

void parent_function(int amount)
{
    struct sigaction act;
    act.sa_handler = sig_parent;
    sigfillset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR1);
    act.sa_flags = 0;
    if (sigaction(SIGINT, &act, NULL) < -1 ||
        sigaction((mode != 3) ? SIGUSR1 : SIGRTMIN, &act, NULL) < -1)
    {
        perror("Parent signal");
        munmap(semaphore, sizeof(sem_t));
        kill(child, 9);
        exit(EXIT_FAILURE);
    }
    sem_wait(semaphore);
    sem_post(semaphore);
    int i;
    for (i = 0; i < amount; i++)
    {
        count1++;
        printf(ANSI_COLOR_BLUE"Parent"ANSI_COLOR_RESET" process: send "ANSI_COLOR_MAGNETA\
            "%s"ANSI_COLOR_RESET" #%i\n",(mode != 3) ? "SIGUSR1" : "SIGRTMIN", count1);
        if (mode != 3) sem_wait_nointr(semaphore);
        kill(child, (mode != 3) ? SIGUSR1 : SIGRTMIN);
        if (mode == 2) sleep(1);
    }
    kill(child, (mode != 3) ? SIGUSR2 : SIGRTMAX);
    alarm(1);
    while(count2 < amount) pause();
    sem_wait(semaphore);
    sem_wait(semaphore);
    printf("Exiting normally.\n");
    munmap(semaphore, sizeof(sem_t));
}

void child_function(void)
{
    struct sigaction act;
    act.sa_handler = sig_child;
    sigfillset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR1);
    act.sa_flags = 0;
    if (sigaction(SIGINT, &act, NULL) < -1 ||
        sigaction((mode != 3) ? SIGUSR1 : SIGRTMIN, &act, NULL) < -1 ||
        sigaction((mode != 3) ? SIGUSR2 : SIGRTMAX, &act, NULL) < -1)
    {
        perror("Child signal");
        exit(EXIT_FAILURE);
    }
    sem_post(semaphore);
    while (1) {pause();}
}

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        printf("Too few arguments!\n");
        exit(EXIT_FAILURE);
    }
    int amount = atoi(argv[1]);
    mode = atoi(argv[2]);
    if (amount == 0 || mode < 1 || mode > 3)
    {
        printf("Incorrect arguments!\n");
        exit(EXIT_FAILURE);
    }
    if ((semaphore = (sem_t*) mmap(NULL, sizeof(sem_t), \
                PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) < 0)
    {
        perror("Semaphore allocation");
        exit(EXIT_FAILURE);
    }
    if (sem_init(semaphore, 1, 1) < 0)
    {
        perror("Semaphore init");
        munmap(semaphore, sizeof(sem_t));
        exit(EXIT_FAILURE);
    }

    sigset_t newmask;
    sigset_t oldmask;
    sigfillset(&newmask);
    sigdelset(&newmask, SIGINT);
    sigdelset(&newmask, SIGUSR1);
    sigdelset(&newmask, SIGUSR2);
    sigdelset(&newmask, SIGRTMIN);
    sigdelset(&newmask, SIGRTMAX);
    if (sigprocmask(SIG_SETMASK, &newmask, &oldmask) < 0)
        perror("Signal mask");

    sem_wait(semaphore);
    parent = getpid();
    count1 = count2 = 0;
    if ((child = fork()) < 0)
    {
        perror("Forking has failed");
        exit(EXIT_FAILURE);
    }
    if (child == 0) child_function();
    else parent_function(amount);
	exit(EXIT_SUCCESS);
}