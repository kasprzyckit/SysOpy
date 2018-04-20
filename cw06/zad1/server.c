#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include "systemv.h"
#include <fcntl.h>
#include <signal.h>

int msgqueue;

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf("%s\n", msg);
    msgctl(msgqueue, IPC_RMID, NULL);
    exit(EXIT_FAILURE);
}

void sigint_handler(int sig)
{
    printf("\nShutting down\n");
    msgctl(msgqueue, IPC_RMID, NULL);
    exit(EXIT_SUCCESS);
}

void set_sigint()
{
    struct sigaction act;
    act.sa_handler = sigint_handler;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGINT, &act, NULL) < -1) err("Signal");
}

int main(int argc, char const *argv[])
{
   // int clients[MAX_CLIENTS];
    set_sigint();
    struct msgbuf msg;
    if ((msgqueue = msgget(ftok("./systemv.h", 0), IPC_CREAT | S_IRWXU)) < 0) err("Server queue open");
    
    for (;;)
    {
        if (msgrcv(msgqueue, &msg, MAX_MSG, 0, 0) < 0) err("Server receive");
        printf("%ld %s\n", msg.mtype, msg.mtext);
    }

    if (msgctl(msgqueue, IPC_RMID, NULL) < 0) err("Server queue rm");
    printf("Exiting\n");
    exit(EXIT_SUCCESS);
}