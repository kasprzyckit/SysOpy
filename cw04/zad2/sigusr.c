#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/mman.h>

#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_MAGNETA "\x1b[35m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int litter;
int threshold;
int trigger;
pid_t* pending;
sem_t* semaphore;

void ws_offset(int i){for(;i>0;i--)printf("  ");}

void sigint_parent(int sig)
{
	if (sig == SIGINT)
	{
		printf("\nTerminating the job.\n");
    	free(pending);
		kill(getpgrp() * (-1), 9);
	}
}

void sig_parent(int sig, siginfo_t *siginfo, void *ucontex)
{
	int psig = siginfo->si_pid;
	int ws = psig- getpid();
	if (psig == getpid()) return;
	if (sig == SIGUSR1)
	{
		printf("Received "ANSI_COLOR_CYAN"SIGUSR1"ANSI_COLOR_RESET":\t");
		ws_offset(ws);
		printf(ANSI_COLOR_BLUE"%i"ANSI_COLOR_RESET"\n", psig);
		if (trigger)
		{

			trigger--;
			pending[trigger] = psig;
			if (trigger == 0)
			{
				int i;
				for (i = 0; i<threshold; i++)
				{
					ws = pending[i] - getpid();
					printf("Granting "ANSI_COLOR_MAGNETA"RT request"ANSI_COLOR_RESET":\t");
					ws_offset(ws);
					printf(ANSI_COLOR_BLUE"%i"ANSI_COLOR_RESET"\n", pending[i]);
					kill(pending[i], SIGUSR1);
				}
			}
		}
		else
		{
			printf("Granting "ANSI_COLOR_MAGNETA"RT request"ANSI_COLOR_RESET":\t");
			ws_offset(ws);
			printf(ANSI_COLOR_BLUE"%i"ANSI_COLOR_RESET"\n", psig);
			kill(psig, SIGUSR1);
		}
	}
	else if (sig >= SIGRTMIN && sig <= SIGRTMAX)
	{
		litter--;
		printf("Received "ANSI_COLOR_GREEN"SIGMIN+%i"ANSI_COLOR_RESET":\t", sig - SIGRTMIN);
		ws_offset(ws);
		printf(ANSI_COLOR_BLUE"%i"ANSI_COLOR_RESET"\n", psig);
	}
	else printf("??? %i\n", sig);

	sem_post(semaphore);
}

void sigusr_child(int sig)
{
	sem_wait(semaphore);
	kill(getppid(), SIGRTMIN + rand()%32);
}

int child_function()
{
	int seed;
    time_t tt;
    seed = time(&tt);
    srand(seed + getpid());

	if (signal(SIGUSR1, sigusr_child) == SIG_ERR)
	{
		perror("SIGUSR1 child");
		exit(EXIT_FAILURE);
	}

	sigset_t newmask;
	sigset_t oldmask;
	sigfillset(&newmask);
	sigdelset(&newmask, SIGUSR1);
	if (sigprocmask(SIG_SETMASK, &newmask, &oldmask) < 0)
		perror("Child mask");

	int rsl = rand()%10 + 1;
	sleep(rsl);
	
	sem_wait(semaphore);
	kill(getppid(), SIGUSR1);
	
	sigsuspend(&newmask);

	printf("Returning value "ANSI_COLOR_RED"%i"ANSI_COLOR_RESET":\t", rsl);
	ws_offset(getpid() - getppid());
	printf(ANSI_COLOR_BLUE"%i"ANSI_COLOR_RESET"\n", getpid());
	
	return rsl;
}

void parent_function()
{
	int seed;
    time_t tt;
    seed = time(&tt);
    srand(seed + getpid());

    struct sigaction act;
    act.sa_handler = sigint_parent;
	sigfillset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGUSR1);
	act.sa_flags = 0;
	if (sigaction(SIGINT, &act, NULL) < -1)
	{
		perror("Parent SIGINT");
    	free(pending);
		kill(getpgrp() * (-1), 9);
		exit(EXIT_FAILURE);
	}
	act.sa_handler = NULL;
	sigemptyset(&act.sa_mask);
	act.sa_sigaction = sig_parent;
	act.sa_flags = SA_SIGINFO | SA_NODEFER;
	if (sigaction(SIGUSR1, &act, NULL) < -1)
	{
		perror("Parent SIGUSR1");
    	free(pending);
		kill(getpgrp() * (-1), 9);
		exit(EXIT_FAILURE);
	}
	int i;
	for (i=SIGRTMIN; i<=SIGRTMAX; i++)
	{
		if (sigaction(i, &act, NULL) < -1)
		{
			perror("Parent SIGRT");
   			free(pending);
			kill(getpgrp() * (-1), 9);
			exit(EXIT_FAILURE);
		}
	}
	sigset_t mask;
	sigemptyset(&mask);

	while (litter > 0)
	{
		sigsuspend(&mask);
	}
	sem_destroy(semaphore);
	printf("Exiting normally\n");
}

int main(int argc, char const *argv[])
{
	if (argc < 3)
	{
		printf("Too few arguments!\n");
		exit(EXIT_FAILURE);
	}
	litter = atoi(argv[1]);
	threshold = atoi(argv[2]);
	if (litter <= 0 || threshold <= 0 || threshold > litter)
	{
		printf("Incorrect arguments!\n");
		exit(EXIT_FAILURE);
	}
	trigger = threshold;

	int i;
    pid_t tmp;
    pending = malloc(threshold * sizeof(pid_t));

    semaphore = (sem_t*) mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    sem_init(semaphore, 1, 1);

    if (getpgrp() != getpid())
    {
    	if (setsid() < 0)
    	{
    		perror("");
    		exit(EXIT_FAILURE);
    	}
    }
    for (i = 0; i < litter; i++)
    {
    	if ((tmp = fork()) < 0)
    	{
    		perror("Fork error!");
    		exit(EXIT_FAILURE);
    	}
    	if (tmp == 0) break;
    	printf("Created process:\t");
    	ws_offset(tmp - getpid());
    	printf(ANSI_COLOR_BLUE"%i"ANSI_COLOR_RESET"\n", tmp);
    }

    if (tmp == 0) exit(child_function());
    else parent_function();
    free(pending);
	exit(EXIT_SUCCESS);
}