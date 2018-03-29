#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

pid_t child;

void sig_stp(int signum)
{
	printf("\n");
	if (kill(child, 0) < 0)
	{
		if (errno == ESRCH)
		{
			if ((child = fork()) < 0)
			{
				printf("Fork error!\n");
				exit(EXIT_FAILURE);
			}
			if (child == 0)
			{
				sigset_t newmask;
				sigset_t oldmask;
				sigemptyset(&newmask);
				sigaddset(&newmask, SIGINT);
				sigprocmask(SIG_BLOCK, &newmask, &oldmask);
				execl("./date.sh", "date.sh", NULL);
				perror("Date proccess");
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			perror("Post-signal fork");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		if (kill(child, 9) == 0)
		{
			int st;
			waitpid(child, &st, 0);
		}
		printf("Oczekuję na CTRL+Z - kontynuacja albo CTR+C - zakonczenie programu.\n");
	}
}

void sig_int(int signum)
{
	if (kill(child, 0) == 0) return;
	printf("\n");
	kill(child, 9);
	exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[])
{
	if (signal(SIGINT, sig_int) == SIG_ERR)
	{
		perror("SIGINT error");
		exit(EXIT_FAILURE);
	}

	struct sigaction act;
	act.sa_handler = sig_stp;
	sigfillset(&act.sa_mask);
	sigdelset(&act.sa_mask, SIGTSTP);
	sigdelset(&act.sa_mask, SIGINT);
	act.sa_flags = 0;
	sigaction(SIGTSTP, &act, NULL);

	if ((child = fork()) < 0)
	{
		printf("Fork error!\n");
		exit(EXIT_FAILURE);
	}
	if (child == 0)
	{
		sigset_t newmask;
		sigset_t oldmask;
		sigemptyset(&newmask);
		sigaddset(&newmask, SIGINT);
		sigprocmask(SIG_BLOCK, &newmask, &oldmask);
		execl("./date.sh", "date.sh", NULL);
		perror("Date proccess");
		exit(EXIT_FAILURE);
	}
	while (1) pause();
	return 0;
}