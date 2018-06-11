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
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "calc.h"

#define ANSI_RED     "\x1b[91m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"

int socket_fd, mode;
char name[MAX_MSG];
int id;

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf(ANSI_RED"%s\n"ANSI_RESET, msg);
    exit(EXIT_FAILURE);
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

void send_packet(msg_t msg_type, char * msg, int cnt)
{
    void *buff;
    packet_t pck;
    pck.msg_type = msg_type;
    pck.msg_length = strlen(msg) + 1;
    pck.c_count = cnt;
    pck.id = id;

    buff = malloc(sizeof(packet_t) + MAX_MSG);
    memcpy(buff, &pck, sizeof(packet_t));
    memcpy(buff+sizeof(packet_t), msg, pck.msg_length);

    if (send(socket_fd,buff,sizeof(packet_t) + pck.msg_length,0) <  0) perror("send");

    free(buff);
}

void receive_packet(char* msg, packet_t* pck)
{
    void *buff = malloc(sizeof(packet_t) + MAX_MSG);
    if (recv(socket_fd,buff,sizeof(packet_t) + MAX_MSG, 0)<0)
        perror("rcv");

    memcpy(pck, buff, sizeof(packet_t));
    memcpy(msg, buff+sizeof(packet_t), pck->msg_length);

    free(buff);
}

void* network(void *arg)
{
    char buff[MAX_MSG];
    packet_t pck;

    for (;;)
    {
        receive_packet(buff, &pck);

        switch(pck.msg_type)
        {
            case SERVER_PING:
                send_packet(CLIENT_PONG, "Pong", -1);
                break;
            case SERVER_CALC:
                printf("Calculating [%i]\n", pck.c_count);
                if(calculate(buff)) send_packet(CLIENT_ANSW, buff, pck.c_count);
                else
                {
                    sprintf(buff, "Incorrect calculation!");
                    send_packet(CLIENT_ERR, buff, -1);
                }
                break;
            default:
                break;            
        }
    }
    return (void*) 0;
}

void parse_args(int argc, char const *argv[], int* port, char* server_addr)
{
    if (argc < 4) err("Too few arguments!");
    sprintf(name, "%s", argv[1]);
    sprintf(server_addr, "%s", argv[3]);
    if (strcmp(argv[2], "INET") == 0)
    {
        mode = 0;
        *port = atoi(argv[4]);
        if (*port == 0 && argv[4][0] != '0') err("Invalid TCP port!");
    }
    else if (strcmp(argv[2], "UNIX") == 0) mode = 1;
    else err("Invalid connection mode!");
}


void init_socket(int port, char* server_addr)
{
    if (mode == 0)
    {
        if((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) err("Inet socket"); 

        struct sockaddr_in inet_addr;
        inet_addr.sin_family = AF_INET;
        inet_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, server_addr, &(inet_addr.sin_addr)) < 0) 
                    err("Incorrect server address!");
        memset(inet_addr.sin_zero, '\0', sizeof(inet_addr.sin_zero));
        if (connect(socket_fd, (struct sockaddr*) &inet_addr, 
            sizeof(struct sockaddr_in))<0) err("Iniet connect");
    }
    else
    {
        if((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) err("Unix socket");

        struct sockaddr_un unix_addr;
        memset(&unix_addr, 0, sizeof(struct sockaddr_un));
        unix_addr.sun_family = AF_UNIX;
        strncpy(unix_addr.sun_path, server_addr,  sizeof(unix_addr.sun_path));
        
        if (bind(socket_fd, (struct sockaddr*) &unix_addr, 
            sizeof(sa_family_t))<0) err("Unix autobind");
        if (connect(socket_fd, (struct sockaddr*) &unix_addr, 
            sizeof(struct sockaddr_un))<0) err("Unix connect");
    }

    packet_t pck;
    char buff[MAX_MSG];

    send_packet(CLIENT_REGISTER, name, -1);
    receive_packet(buff, &pck);

    if (pck.msg_type == SERVER_ACCEPT)
    {
        printf("%s\n", buff);
        id = pck.id;
    }
    else err(buff);
}

void __exit(void)
{
    send_packet(CLIENT_UNREG, name, -1);
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
}

int main(int argc, char const *argv[])
{
    char server_addr[100];
    int port;
    parse_args(argc, argv, &port, server_addr);
    init_socket(port, server_addr);
    atexit(__exit);

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

    return 0;
}