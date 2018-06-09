#define _BSD_SOURCE
#define _GNU_SOURCE
#define _SVID_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../calc.h"

#define ANSI_RED     "\x1b[91m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_MAGNETA "\x1b[35m"
#define ANSI_RESET   "\x1b[0m"

char name[MAX_MSG];
int mode;
struct sockaddr* server_address = NULL;
int server_fd;

void* network(void *arg);
void init_connection(void);
void parse_args(int argc, char const *argv[]);
void __exit(void);
void err(const char* msg);

int send_packet(msg_t msg_type, char* msg)
{
    void *buff;
    packet_t pck;
    pck.msg_type = msg_type;
    pck.msg_length = strlen(msg) + 1;

    buff = malloc(sizeof(packet_t) + pck.msg_length);
    memcpy(buff, &pck, sizeof(packet_t));
    memcpy(buff+sizeof(packet_t), msg, pck.msg_length);

    if (write(server_fd, buff, 
        sizeof(packet_t) + pck.msg_length) < 0) return -1;
    return 0;
}

int calculate(char* msg)
{
    char opr, tmp[10];
    int c = 0, n1, n2, res, tm, len = strlen(msg);

    while (msg[c] == ' ' && c < len) c++;
    tm = c;
    while (msg[c] > '0' && msg[c] <= '9' && c < len) c++;
    if (c == len || c == tm) return 0;
    sprintf(tmp, "%.*s",(c-tm > 10) ? c-tm : 10, msg+tm);
    n1 = atoi(tmp);
    while (msg[c] == ' ' && c < len) c++;
    if (c == len) return 0;
    opr = msg[c++];
    while (msg[c] == ' ' && c < len) c++;
    if (c == len || msg[c] < '0' || msg[c] > '9') return 0;
    sprintf(tmp, "%.*s", (len-c > 10) ? len-c : 10, msg + c);
    n2 = atoi(tmp);
    switch (opr)
    {
        case '+': res = n1 + n2; break;
        case '-': res = n1 - n2; break;
        case '/': if (n2) res = n1 / n2; else return 0; break;
        case '*': res = n1 * n2; break;
        default: return 0;
    }
    sprintf(msg, "%i", res);
    return 1;
}

void* network(void *arg)
{
    char buff[MAX_MSG];
    packet_t pck;

    for (;;)
    {
        read(server_fd, &pck, sizeof(packet_t));
        read(server_fd, buff, pck.msg_length);

        switch(pck.msg_type)
        {
            case SERVER_PING:
                send_packet(CLIENT_PONG, "Pong");
                break;
            case SERVER_CALC:
                if (calculate(buff)) send_packet(CLIENT_ANSW, buff);
                else
                {
                    sprintf(buff, "Incorrect calculation!");
                    send_packet(CLIENT_ERR, buff);
                }
                break;
            default:
                break;            
        }
    }
    return (void*) 0;
}

int main(int argc, char const *argv[])
{
    parse_args(argc, argv);

    atexit(__exit);
    init_connection();

    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    pthread_t pth;
    pthread_create(&pth, NULL, &network, NULL);

    sigwait(&set, &sig);
    pthread_cancel(pth);
    exit(EXIT_SUCCESS);
}

void init_connection(void)
{
    char buff[MAX_MSG];
    packet_t pck;

    if (mode == 0)
    {
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            err("Inet socket");
        if (connect(server_fd, server_address, sizeof(struct sockaddr_in)) < 0)
            err("Inet connection to server");
    }
    else
    {
        if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
            err("UNIX socket");
        if (connect(server_fd, server_address, sizeof(struct sockaddr_un)) < 0)
            err("UNIX connection to server");
    }

    send_packet(CLIENT_REGISTER, name);

    read(server_fd, &pck, sizeof(packet_t));
    read(server_fd, buff, pck.msg_length);

    if (pck.msg_type == SERVER_ACCEPT) printf("%s\n", buff);
    else err(buff);
}

void parse_args(int argc, char const *argv[])
{
    if (argc < 4) err("Too few arguments!");
    sprintf(name, "%s", argv[1]);
    if (strcmp(argv[2], "INET") == 0)
    {
        struct sockaddr_in* tmp;
        mode = 0;
        tmp = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
        memset(tmp, 0, sizeof(struct sockaddr_in));
        tmp->sin_family = AF_INET;
        tmp->sin_port = htons((uint16_t)atoi(argv[4]));
        if (inet_pton(AF_INET, argv[3], &(tmp->sin_addr)) < 0) 
            err("Incorrect server address!");
        server_address = (struct sockaddr*)tmp;
    }
    else if (strcmp(argv[2], "UNIX") == 0)
    {
        struct sockaddr_un* tmp;
        mode = 1;
        tmp = (struct sockaddr_un*) malloc(sizeof(struct sockaddr_un));
        memset(tmp, 0, sizeof(struct sockaddr_un));
        tmp->sun_family = AF_UNIX;
        strncpy(tmp->sun_path, argv[3], 
            sizeof(tmp->sun_path) - 1);
        server_address = (struct sockaddr*)tmp;
    }
    else err("Invalid connection mode!");
}

void __exit(void)
{
    send_packet(CLIENT_UNREG, name);
    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
    if (server_address != NULL) free(server_address);
}

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf(ANSI_RED"%s\n"ANSI_RESET, msg);
    exit(EXIT_FAILURE);
}