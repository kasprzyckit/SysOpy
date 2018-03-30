#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

int litter;
int threshold;
int trigger;
pid_t* pending;

/*void sigint_parent(int sig)
{
	printf("kill %i %i\n", getpid(), getpgrp());
	if (sig == SIGINT)
	{
		printf("Terminating the job.\n");
    	free(pending);
		//kill(getpgrp() * (-1), 9);
		exit(EXIT_SUCCESS);
	}

}*/

void sig_parent(int sig, siginfo_t *siginfo, void *ucontex)
{
	if (siginfo->si_pid == getpid()) return;
	if (sig == SIGUSR1)
	{
//		printf("1\n");
		printf("Received SIGUSR1 from %i\n", siginfo->si_pid);
		if (trigger)
		{
//			printf("2\n");
			trigger--;
//			printf("tt %i\n", trigger);
			pending[trigger] = siginfo->si_pid;
			if (trigger == 0)
			{
//				printf("3\n");
				int i;
				for (i = 0; i<threshold; i++)
				{
					printf("Granting RT request: %i\n", pending[i]);
					kill(pending[i], SIGUSR1);
				}
			}
		}
		else
		{
//			printf("4\n");
			printf("Granting RT request: %i\n", siginfo->si_pid);
			kill(siginfo->si_pid, SIGUSR1);
		}
	}
	else if (sig >= SIGRTMIN && sig <= SIGRTMAX)
	{
		litter--;
//		printf("ll %i\n", litter);
//		printf("5\n");
		printf("Received SIGMIN+%i from %i\n", sig - SIGRTMIN, siginfo->si_pid);
	}
	else printf("??? %i\n", sig);
}

void sigusr_child(int sig)
{
	int tt = SIGRTMIN + rand()%32;
//	printf("Sending rt %i: %i\n", tt, getpid());
	kill(getppid(), tt);
}

int child_function()
{
	int seed;
    time_t tt;
    seed = time(&tt);
    srand(seed + getpid());

//	printf("%i %i\n", getpid(), getpgrp());
	if (signal(SIGUSR1, sigusr_child) == SIG_ERR)
	{
		perror("SIGUSR1 child");
		exit(EXIT_FAILURE);
	}

	sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask, SIGUSR1);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
		perror("Child mask");

	int rsl = rand()%10 + 1;
//	printf("sleeping for %i: %i\n", rsl, getpid());
	sleep(rsl);
//	printf("waked up: %i\n", getpid());

	kill(getppid(), SIGUSR1);
	sigsuspend(&mask);

	printf("Returning with value %i: %i\n", rsl, getpid());
	return rsl;
}

void parent_function()
{
	int seed;
    time_t tt;
    seed = time(&tt);
    srand(seed + getpid());

 //   signal(SIGINT, sigint_parent);

	struct sigaction act;
	act.sa_sigaction = sig_parent;
	sigfillset(&act.sa_mask);
//	sigdelset(&act.sa_mask, SIGINT);
	act.sa_flags = SA_SIGINFO;
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
	while (litter > 0)
	{
//		printf("asdasd %i %i\n", litter, trigger);
		pause();
	}
	printf("Exiting normally\n");
	kill(getpgrp() * (-1), 9);
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

    if (getpgrp() != getpid())
    {
    	if (setsid() < 0)
    	{
    		perror("");
    		exit(EXIT_FAILURE);
    	}
    }
//    printf("ddd %i %i\n", litter, trigger);
    for (i = 0; i < litter; i++)
    {
    	if ((tmp = fork()) < 0)
    	{
    		perror("Fork error!");
    		exit(EXIT_FAILURE);
    	}
    	if (tmp == 0) break;
    	printf("Created child process: %i\n", tmp);
    }

    if (tmp == 0) exit(child_function());
    
    parent_function();
    free(pending);
	return 0;
}