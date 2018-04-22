#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include "posix.h"

#define ANSI_RED     "\x1b[91m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"

mqd_t msgqueue;
mqd_t server;
int client_id = -1;
char queue_name[8];

void __exit();
void err(const char* msg);
void sig_handler(int sig);
void set_sigint();
char parse_command(char *line, int len, int* pos);
void connect_to_server();
void receive_reply(char* msg);

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
    char msg[MAX_MSG];
    int count, pos;
    char msg_id;

    sprintf(queue_name, "/%iq", getpid());

    if ((server = mq_open(SERVER_NAME, O_WRONLY)) < 0) err("Client->server queue");
    if ((msgqueue = mq_open(queue_name, O_CREAT | O_EXCL | O_RDONLY, S_IRUSR | S_IWUSR, NULL)) < 0) err("Server->client queue");
    
    connect_to_server(queue_name);
    while (printf(ANSI_BLUE"> "ANSI_RESET) < 0 || (count = getline(&buf, &n, file)) > 1)
    {
        sprintf(line, "%.*s", (buf[count-1] == '\n') ? count-1 : count, buf);
        msg_id = parse_command(line, count, &pos);
        
        if (msg_id == UNDEF_MSG)
        {
            printf(ANSI_RED"Command not recognized: %s\n"ANSI_RESET, line);
            continue;
        }
        
        msg[0] = msg_id;
        sprintf(msg+2, "%s", line + pos);
        msg[1] = (char) client_id;
        if (mq_send(server, msg, MAX_MSG, 1) < 0)
        {
            if (errno == EBADF && intr)
            {
                printf(ANSI_RED"Server not found\n"ANSI_RESET);
                continue;
            }
            else err("Client send");
        }
        
        receive_reply(msg);
    }

    printf("Exiting\n");
    exit(EXIT_SUCCESS);
}

char parse_command(char *line, int len, int* pos)
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

void connect_to_server(char* queue_name)
{
    char msg[MAX_MSG];

    msg[0] = INIT_MSG;
    msg[1] = -1;
    sprintf(msg+2, "%s", queue_name);
    if (mq_send(server, msg, MAX_MSG, 1) < 0) err("Client send init");

    if (mq_receive(msgqueue, msg, MAX_MSG, NULL) < 0) err("Client receive init");
    switch(msg[0])
    {
        case RPLY_MSG:
            client_id = atoi(msg+2);
            if (client_id < 0) err("Client register");
            break;
        case ERR_MSG:
            err(msg+2);
            break;
        default:
            break;
    }
}

void receive_reply(char* msg)
{
    if (msg[0] == END_MSG) return;
    if (mq_receive(msgqueue, msg, MAX_MSG, NULL) < 0) perror("Client receive");
    printf("%s\n", msg+2);
}

void __exit(void)
{
    if (client_id != -1)
    {
        char msg[MAX_MSG];
        msg[0] = STOP_MSG;
        msg[1] = (char)client_id;
        sprintf(msg+2, "%i", client_id);
        if (mq_send(server, msg, MAX_MSG, 1) < 0) perror("Exit send");
    }

    if (mq_close(server) < 0) perror("Exit close server");
    if (mq_close(msgqueue) < 0) perror("Exit close client");
    if (mq_unlink(queue_name) < 0) perror("Exit unlink client");
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