#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include "../zad2/words.h"

#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_MAGNETA "\x1b[35m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void print_time(const char* mes, time_t secs, suseconds_t msecs)
{
	if (msecs < 0)
	{
		secs -= 1;
		msecs += 1000000;
	}
	printf("%s: %ld second(s) %ld microsecond(s)\n", mes, secs, msecs);
}

void print_usage(struct rusage *usage_old, struct rusage *usage_new)
{
	printf("\n");
	print_time(ANSI_COLOR_MAGNETA "system time" ANSI_COLOR_RESET, \
		usage_new->ru_stime.tv_sec - usage_old->ru_stime.tv_sec, \
		usage_new->ru_stime.tv_usec - usage_old->ru_stime.tv_usec);
	print_time(ANSI_COLOR_MAGNETA "user time" ANSI_COLOR_RESET, \
		usage_new->ru_utime.tv_sec - usage_old->ru_utime.tv_sec, \
		usage_new->ru_utime.tv_usec - usage_old->ru_utime.tv_usec);
	printf(ANSI_COLOR_MAGNETA "resident set size: ");
	if (usage_old->ru_maxrss == usage_new->ru_maxrss) printf(ANSI_COLOR_RESET "<");
	printf(ANSI_COLOR_RESET "%ld kilobytes\n", usage_new->ru_maxrss);
}

void duplicate_usage(struct rusage *usage_old, struct rusage *usage_new)
{
	usage_old->ru_stime.tv_sec = usage_new->ru_stime.tv_sec;
	usage_old->ru_stime.tv_usec = usage_new->ru_stime.tv_usec;
	usage_old->ru_utime.tv_sec = usage_new->ru_utime.tv_sec;
	usage_old->ru_utime.tv_usec = usage_new->ru_utime.tv_usec;
	usage_old->ru_maxrss = usage_new->ru_maxrss;
}

int main(int argc, char const *argv[])
{

	if (argc < 4)
	{
		printf("Too few arguments!\n");
		exit(EXIT_FAILURE);
	}

	int cpu_limit = atoi(argv[2]);
    int as_limit = atoi(argv[3]);
    if (cpu_limit == 0 || as_limit == 0)
    {
    	printf("Incorrect arguments!\n");
		exit(EXIT_FAILURE);
    }
    printf("Current limits:\nCPU time: %i s\nMemory: %i MB\n", cpu_limit, as_limit);

	FILE *fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
    	perror(argv[1]);
    	exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    int count = 0;
    pid_t child_pid;
    int sl;
    int not_found = 0;
    struct rusage *usage_old = malloc(sizeof(struct rusage));
    struct rusage *usage_new = malloc(sizeof(struct rusage));
    getrusage(RUSAGE_CHILDREN, usage_old);

    struct rlimit *limits = malloc(sizeof(struct rlimit));

    while (getline(&line, &len, fp) != -1)
	{
		count++;
		child_pid = fork();
		if (child_pid < 0)
		{
			perror(line);
			break;
		}
		if (child_pid == 0)
		{
			limits->rlim_cur = limits->rlim_max = (rlim_t) cpu_limit;
    		if (setrlimit(RLIMIT_CPU, limits) < 0)
    		{
    			perror(line);
    			break;
    		}

			limits->rlim_cur = limits->rlim_max = (rlim_t) as_limit*1024*1024;
    		if (setrlimit(RLIMIT_AS, limits) < 0)
    		{
    			perror(line);
    			break;
    		}

			words_list wl = getwords(strdup(line));
			free(line);
			free(usage_old);
			free(usage_new);
			free(limits);
    		fclose(fp);

    		execvp(wl.list[0], wl.list);
			not_found = 1;
			break;
		}
		else
		{
			printf(ANSI_COLOR_CYAN"\nin[%i]$" ANSI_COLOR_BLUE" %s" ANSI_COLOR_RESET "\n", count, line);
			if (waitpid(child_pid, &sl, WUNTRACED) < 0)
			{
				perror(line);
				break;
			}
			getrusage(RUSAGE_CHILDREN, usage_new);
			if (WEXITSTATUS(sl))
			{
				printf(ANSI_COLOR_RED "Job #%i has failed!"ANSI_COLOR_RESET "\n", count);
				break;
			}
			else
			{
				if (WIFSIGNALED(sl))
				{
					printf(ANSI_COLOR_RED "\nJob #%i has been terminated or reached one of its resource usage limit!" ANSI_COLOR_RESET "\n", count);
					break;
				}
				print_usage(usage_old, usage_new);
				duplicate_usage(usage_old, usage_new);
			}
		}
    }

    if (not_found) exit(EXIT_FAILURE);
    fclose(fp);
	if (line != NULL) free(line);
	free(usage_old);
	free(usage_new);
	free(limits);
	if (! errno) exit(EXIT_SUCCESS);
	exit(EXIT_SUCCESS);
}
