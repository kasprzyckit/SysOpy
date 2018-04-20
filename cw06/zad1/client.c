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
#include <string.h>

int msgqueue;
int server;
int client_id = -1;

void shut_down()
{
    struct msgbuf msg;
    msg.mtype = STOP_MSG;
    sprintf(msg.mtext, "%i", client_id);
    msgsnd(server, &msg, MAX_MSG, 0);
}

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf("%s\n", msg);
    msgctl(msgqueue, IPC_RMID, NULL);
    shut_down();
    exit(EXIT_FAILURE);
}

void sigint_handler(int sig)
{
    printf("\nShutting down\n");
    msgctl(msgqueue, IPC_RMID, NULL);
    shut_down();
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

int parse_command(char *line, int len, int* pos)
{
    int c = 0;
    int ret;
    char cmnd[6];

    while (line[c] != ' ' && c != len-1 && c < 6) c++;
    sprintf(cmnd, "%.*s", c, line);
    if (line[c] == ' ') while(line[c] == ' ') c++;
    *pos = c;
    
    if (strcmp(cmnd, "MIRROR") == 0)
        ret = MIRROR_MSG;
    else if (strcmp(cmnd, "CALC") == 0)
        ret = CALC_MSG;
    else if (strcmp(cmnd, "TIME") == 0)
        ret = TIME_MSG;
    else if (strcmp(cmnd, "END") == 0)
        ret = END_MSG;
    else ret = UNDEF_MSG;

    return ret;
}

int main(int argc, char const *argv[])
{
    FILE *file;
    if (argc > 1)
    {
        file = fopen(argv[1], "r");
        if (file == NULL) err("Commands file");
    }
    else file = stdin;

    set_sigint();

    char *buf = NULL;
    char line[100];
    size_t n;
    struct msgbuf msg;
    int count, pos, msg_id;

    if ((server = msgget(ftok("./systemv.h", 0), 0)) < 0) err("Client->server queue");
    if ((msgqueue = msgget(IPC_PRIVATE, 0)) < 0) err("Server->client queue");

    msg.mtype = INIT_MSG;
    sprintf(msg.mtext, "%i", msgqueue);
    if (msgsnd(server, &msg, MAX_MSG, 0) < 0) err("Client init");
    if (msgrcv(msgqueue, &msg, MAX_MSG, INIT_MSG, S_IRWXU) < 0) err("Client receive init");
    client_id = atoi(msg.mtext);
    printf("ddd %i\n", client_id);
    
    while ((count = getline(&buf, &n, file)) > 1)
    {
        sprintf(line, "%.*s", count-1, buf);
        msg_id = parse_command(line, count, &pos);
        
        if (msg_id == UNDEF_MSG)
        {
            printf("Command not recognized\n");
            continue;
        }
        msg.mtype = msg_id;
        sprintf(msg.mtext, "%s", line + pos);
        if (msgsnd(server, &msg, MAX_MSG, 0) < 0) err("Client send");
    }

    msgctl(msgqueue, IPC_RMID, NULL);
    shut_down();
    printf("Exiting\n");
    exit(EXIT_SUCCESS);
}