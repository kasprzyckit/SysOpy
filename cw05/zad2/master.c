#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_BUF 1024

void err(char* msg)
{
    if (errno) perror(msg);
    else printf("%s\n", msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char const *argv[])
{
    if (argc < 2) err("Too few arguments!");

    if (mkfifo(argv[1], S_IRWXU | S_IRWXG | S_IRWXO) < 0) 
    {
        if (errno == EEXIST) errno = 0;
        else err("Master pipe");
    }
   
    int fd, count;
    char buf[MAX_BUF];
    printf("\nMaster init\n");

    if ((fd = open(argv[1], O_RDONLY)) < 0) err("Master pipe");
    while ((count = read(fd, buf, MAX_BUF)) > 0)
    {
        printf("%.*s", count, buf);
        strcpy(buf, "");
    }

    close(fd);
    printf("Master exit\n");
    exit(EXIT_SUCCESS);
}