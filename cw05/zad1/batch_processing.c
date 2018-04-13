#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include "words.h"

#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void err(const char* msg);

int main(int argc, char const *argv[])
{
	if (argc < 2) err("Too few arguments.");
    
    int i;
	FILE *fp;
    char *line = NULL;
    size_t len = 0;
    int sl, count = 0;
    int fd[MAX_ARG - 1][2];
    pid_t child_pid[MAX_ARG];
    exp_list el;

	if ((fp = fopen(argv[1], "r")) == NULL) err(argv[1]);

	while (getline(&line, &len, fp) != -1)
	{
        printf(ANSI_COLOR_CYAN"\nin[%i]$" ANSI_COLOR_BLUE \
            " %s" ANSI_COLOR_RESET "\n", count++, line);
        el = tokenize(line);

        for (i = 0; i < el.length; i++)
        {
            if (i != el.length-1)
                if ((pipe(fd[i])) < 0) err(el.wlist[i].list[0]);
            if ((child_pid[i] = fork()) < 0) err(line);
            if (child_pid[i] == 0)
            {
                if (i)
                {
                    dup2(fd[i-1][0], STDIN_FILENO);
                    close(fd[i-1][1]);
                }
                if (i != el.length-1)
                {
                    dup2(fd[i][1], STDOUT_FILENO);
                    close(fd[i][0]);
                }

                execvp(el.wlist[i].list[0], el.wlist[i].list);
                _exit(EXIT_FAILURE);
            }
            if (i) close(fd[i-1][0]);
            if (i != el.length-1) close(fd[i][1]);            
        }
        for (i = 0; i < el.length; i++)
        {
            if (waitpid(child_pid[i], &sl, 0) < 0) err(line);
            if (!WIFEXITED(sl) || WEXITSTATUS(sl))
            {
                printf(ANSI_COLOR_RED "Job #%i has failed!"ANSI_COLOR_RESET "\n", count);
                break;
            }
        }
        if (errno) break;
    }

	if (line != NULL) free(line);
    fclose(fp);
    if (errno) exit(EXIT_FAILURE);
	exit(EXIT_SUCCESS);
}

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf("%s\n", msg);
    killpg(getpgrp(), SIGINT);
}