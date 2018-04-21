#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include "systemv.h"

#define ANSI_RED     "\x1b[91m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"

int msgqueue;
int server;
int client_id = -1;

void __exit();
void err(const char* msg);
void sig_handler(int sig);
void set_sigint();
int parse_command(char *line, int len, int* pos);
void connect_to_server();
void receive_reply(struct msgbuf* msg);

int main(int argc, char const *argv[])
{
    FILE *file;
    int intr = 1;
    if (argc > 1)
    {
        file = fopen(argv[1], "r");
        if (file == NULL) err("Commands file");
        intr = 0;
    }
    else file = stdin;

    set_sigint();
    atexit(__exit);

    char *buf = NULL;
    char line[100];
    size_t n;
    struct msgbuf msg;
    int count, pos, msg_id;

    if ((server = msgget(ftok(getenv("HOME"), 0), 0)) < 0) err("Client->server queue");
    if ((msgqueue = msgget(IPC_PRIVATE, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) err("Server->client queue");
    
    connect_to_server();
    while (printf(ANSI_BLUE"> "ANSI_RESET) < 0 || (count = getline(&buf, &n, file)) > 1)
    {
        sprintf(line, "%.*s", (buf[count-1] == '\n') ? count-1 : count, buf);
        msg_id = parse_command(line, count, &pos);
        
        if (msg_id == UNDEF_MSG)
        {
            printf(ANSI_RED"Command not recognized: %s\n"ANSI_RESET, line);
            continue;
        }
        
        msg.mtype = msg_id;
        sprintf(msg.mtext, "%s", line + pos);
        msg.client_id = client_id;
        msg.client_pid = getpid();
        if (msgsnd(server, &msg, MAX_MSG, 0) < 0)
        {
            if (errno == EINVAL && intr)
            {
                printf(ANSI_RED"Server not found\n"ANSI_RESET);
                continue;
            }
            else err("Client send");
        }
        
        receive_reply(&msg);
    }

    printf("Exiting\n");
    exit(EXIT_SUCCESS);
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

void connect_to_server()
{
    struct msgbuf msg;

    msg.mtype = INIT_MSG;
    msg.client_id = -1;
    msg.client_pid = getpid();
    sprintf(msg.mtext, "%i", msgqueue);
    if (msgsnd(server, &msg, MAX_MSG, 0) < 0) err("Client send init");

    if (msgrcv(msgqueue, &msg, MAX_MSG, 0, 0) < 0) err("Client receive init");
    switch(msg.mtype)
    {
        case RPLY_MSG:
            client_id = atoi(msg.mtext);
            if (client_id < 0) err("Client register");
            break;
        case ERR_MSG:
            err(msg.mtext);
            break;
        default:
            break;
    }
}

void receive_reply(struct msgbuf* msg)
{
    if (msg->mtype == END_MSG) return;
    if (msgrcv(msgqueue, msg, MAX_MSG, 0, MSG_NOERROR) < 0) perror("Client receive");
    printf("%s\n", msg->mtext);
}

void __exit(void)
{
    msgctl(msgqueue, IPC_RMID, NULL);
    if (client_id == -1) return;
    struct msgbuf msg;
    msg.mtype = STOP_MSG;
    msg.client_id = client_id;
    msg.client_pid = getpid();
    sprintf(msg.mtext, "%i", client_id);
    msgsnd(server, &msg, MAX_MSG, 0);
}

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf(ANSI_RED"%s\n"ANSI_RESET, msg);
    exit(EXIT_FAILURE);
}

void sig_handler(int sig)
{
    if (sig == SIGINT)
    {
        printf("\nShutting down\n");
        exit(EXIT_SUCCESS);
    }
    else if (sig == SIGSEGV) err("Segmentation fault");
}

void set_sigint()
{
    struct sigaction act;
    act.sa_handler = sig_handler;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGINT, &act, NULL) < -1) err("Signal");
    if (sigaction(SIGSEGV, &act, NULL) < -1) err("Signal");
}