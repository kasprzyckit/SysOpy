#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "words.h"

#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int main(int argc, char const *argv[])
{
	if (argc < 2)
	{
		printf("Too few arguments!\n");
		exit(EXIT_FAILURE);
	}

	FILE *fp;
    char *line = NULL;
    size_t len = 0;
    int count = 0;
    pid_t child_pid;
    int sl;
    int not_found = 0;

	fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
    	perror(argv[1]);
    	exit(EXIT_FAILURE);
    }

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
			words_list wl = getwords(strdup(line));
			free(line);
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
			if (WEXITSTATUS(sl))
			{
				printf(ANSI_COLOR_RED "Job #%i has failed!"ANSI_COLOR_RESET "\n", count);
				break;
			}
		}
    }

    if (not_found) exit(EXIT_FAILURE);
    fclose(fp);
	if (line != NULL) free(line);
	if (! errno) exit(EXIT_SUCCESS);
	exit(EXIT_SUCCESS);
}
