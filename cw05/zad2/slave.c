#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>

#define ANSI_CYAN    "\x1b[36m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RED     "\x1b[91m"
#define ANSI_MAGNETA "\x1b[35m"
#define ANSI_RESET   "\x1b[0m"

#define MAX_BUF 1024

void err(char* msg)
{
    if (errno) perror(msg);
    else printf("%s\n", msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char const *argv[])
{
    if (argc < 3) err("Too few arguments.");

    int n = atoi(argv[2]);
    if (n <= 0) err("Incorrect arguments.");

    int fd, i, seed;
    char buf[MAX_BUF];
    char datebuf[100];
    time_t tt;
    FILE *date;

    seed = time(&tt);
    srand(seed + getpid());
    sleep(1);

    if ((fd = open(argv[1], O_WRONLY)) < 0) err("Slave pipe");

    printf("Slave init: "ANSI_BLUE"%i"ANSI_RESET"\n", getpid());

    for (i = 0; i < n; i++)
    {
        date = popen("date", "r");
        if (date == NULL || date < 0) err("date");
        fgets(datebuf, MAX_BUF, date);
        pclose(date);

        sprintf(buf, "Slave "ANSI_BLUE"%i"ANSI_RESET": " \
            ANSI_RED"%i" ANSI_RESET" / "ANSI_CYAN"%i\t" \
            ANSI_MAGNETA"%s"ANSI_RESET, getpid(), i+1, n, datebuf);

        if (i == n-1) sprintf(buf, "%sSlave exit: " \
            ANSI_BLUE"%i"ANSI_RESET"\n", buf, getpid());

        lseek (fd, 0, SEEK_SET);
        if (write(fd, buf, strlen(buf)) <= 0) err("pipe");
        sleep(rand()%4 + 2);
    }
    close (fd);

    exit(EXIT_SUCCESS);
}