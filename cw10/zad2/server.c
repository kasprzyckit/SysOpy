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

#define MAX_EVENTS      40
#define MAX_CLIENTS     20

int client_counter = 0;
client_list clist;
int inet_socket;
int unix_socket;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;
char socket_path[100];

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf(ANSI_RED"%s\n"ANSI_RESET, msg);
    exit(EXIT_FAILURE);
}

void send_packet(int socket_fd, struct sockaddr* client_addr, socklen_t addr_size,
    msg_t msg_type, char * msg, int cnt, int id)
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

    if (sendto(socket_fd,buff,sizeof(packet_t) + pck.msg_length,0,
        client_addr,addr_size)<0) perror("send");

    free(buff);
}

void receive_packet(int socket_fd, struct sockaddr * client_addr, socklen_t* addr_size,
    char* msg, packet_t* pck)
{
    void *buff = malloc(sizeof(packet_t) + MAX_MSG);
    if (recvfrom(socket_fd,buff,sizeof(packet_t) + MAX_MSG, 0,
        client_addr, addr_size)<0) perror("rcv");

    memcpy(pck, buff, sizeof(packet_t));
    memcpy(msg, buff+sizeof(packet_t), pck->msg_length);

    free(buff);
}

void accept_message(int socket_fd)
{
    socklen_t addr_size = (socket_fd == inet_socket) ? \
        sizeof(struct sockaddr_in) : sizeof(struct sockaddr_un);
    struct sockaddr* client_address = malloc(addr_size);

    char buff[MAX_MSG];
    packet_t pck;
    
    receive_packet(socket_fd, client_address, &addr_size, buff, &pck);

    switch(pck.msg_type)
    {
        case CLIENT_REGISTER:
            pthread_mutex_lock(&list_mutex);

            if (clist.size >= MAX_CLIENTS)
            {
                pthread_mutex_unlock(&list_mutex);

                send_packet(socket_fd, client_address, addr_size,
                    SERVER_REJECT, "Server full", -1, -1);
                return;
            } 
            if (is_present_clist(&clist, buff))
            {
                pthread_mutex_unlock(&list_mutex);

                send_packet(socket_fd, client_address, addr_size,
                    SERVER_REJECT, "Name already in use", -1, -1);
                return;
            }
            send_packet(socket_fd, client_address, addr_size,
                SERVER_ACCEPT, "CALC cluster server: welcome!", -1, client_counter);

            add_clist(&clist, client_address, addr_size, buff,
                client_counter++, socket_fd);

            pthread_mutex_unlock(&list_mutex);
            break;
        case CLIENT_PONG:
            pthread_mutex_lock(&list_mutex);

            confirm_ping(&clist, pck.id);

            pthread_mutex_unlock(&list_mutex);
            break;
        case CLIENT_ANSW:
            printf("[%i] %s\n"ANSI_BLUE"%i> "ANSI_RESET, \
                pck.c_count, buff, counter);
            fflush(stdout);
            break;
        case CLIENT_ERR:
            printf(ANSI_RED"%s"ANSI_BLUE"\n%i> "ANSI_RESET, buff, counter);
            fflush(stdout);
            break;
        case CLIENT_UNREG:
            pthread_mutex_lock(&list_mutex);

            remove_clist(&clist, pck.id);

            pthread_mutex_unlock(&list_mutex);
            break;
        default:
            break;
    }
    free(client_address);
}

void* network(void *arg)
{
    struct epoll_event ev, events[MAX_EVENTS];
    int epoll_fd, nfds, i;

    if ((epoll_fd = epoll_create1(0))< 0) err("Epoll creation");
    ev.events = EPOLLIN;
    ev.data.fd = inet_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, inet_socket, &ev) < 0)
        err("Epoll server add");
    ev.data.fd = unix_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, unix_socket, &ev) < 0)
        err("Epoll server add");

    for (;;)
    {
        if ((nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1)) < 0)
            err("Epoll wait");

        for (i = 0; i < nfds; i++) accept_message(events[i].data.fd);
    }

    return (void*) 0;
}

void* clients_ping(void *arg)
{
    struct client_node* p;
    struct sockaddr* address;
    socklen_t addr_size;
    for (;;)
    {
        pthread_mutex_lock(&list_mutex);

        for (p = clist.first; p != NULL; p = p->next)
            if (p->ping == 0) remove_clist(&clist, p->id);

        reset_ping(&clist);

        for (p = clist.first; p != NULL; p = p->next)
        {
            address = p->address;
            addr_size = p->addr_size;
            send_packet(p->sock, address, addr_size, 
                SERVER_PING, "Ping", -1, -1);
        }

        pthread_mutex_unlock(&list_mutex);

        sleep(2);
    }

    return (void*) 0;
}

void* terminal(void *arg)
{
    struct sockaddr* address;
    socklen_t addr_size;
    char* line = NULL;
    size_t n = 0;
    int count, sock;
    printf(ANSI_BLUE"0> "ANSI_RESET);
    for (;;)
    {
        count = getline(&line, &n, stdin);
        line[count-1] = '\0';

        pthread_mutex_lock(&list_mutex);
        if (clist.size > 0)
        {
            get_next_address(&clist, &address, &addr_size, &sock);
            send_packet(sock, address, addr_size,
                SERVER_CALC, line, counter++, -1);
        }
        else printf(ANSI_RED"No clients connected!" \
            ANSI_BLUE"\n%i> "ANSI_RESET, counter);
        pthread_mutex_unlock(&list_mutex);

        free(line);
        n = 0;
    }

    return (void*) 0;
}

void parse_args(int argc, char const *argv[], int* port, char* socket_path)
{
    if (argc < 2) err("Too few arguments!");
    *port = atoi(argv[1]);
    if (*port == 0 && argv[1][0] != '0') err("Invalid TCP port!");
    sprintf(socket_path, "%s", argv[2]);
}

void init_socket(int port, char* socket_path)
{
    struct sockaddr_in addr_inet;
    struct sockaddr_un addr_unix;

    if ((inet_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) err("Inet socket");
    addr_inet.sin_family = AF_INET;
    addr_inet.sin_port = htons(port);
    addr_inet.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(addr_inet.sin_zero, '\0', sizeof addr_inet.sin_zero);
    if(bind(inet_socket, (struct sockaddr *) &addr_inet, sizeof(addr_unix)) < 0)
        err("Inet bind");

    unlink(socket_path);
    if ((unix_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) err("Unix socket");
    memset(&addr_unix, 0, sizeof(struct sockaddr_un));
    addr_unix.sun_family = AF_UNIX;
    strncpy(addr_unix.sun_path, socket_path, sizeof(addr_unix.sun_path));
    if(bind(unix_socket, (struct sockaddr *) &addr_unix, sizeof(addr_unix)) < 0)
        err("Unix bind");

    init_clist(&clist);
}

void __exit(void)
{
    shutdown(inet_socket, SHUT_RDWR);
    shutdown(unix_socket, SHUT_RDWR);

    close(inet_socket);
    close(unix_socket);

    unlink(socket_path);

    pthread_mutex_destroy(&list_mutex);

    printf("\n");
}

int main(int argc, char const *argv[])
{
    int port;
    parse_args(argc, argv, &port, socket_path);
    atexit(__exit);
    init_socket(port, socket_path);

    sigset_t set;
    int sig, i;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    pthread_t pth[3];
    pthread_create(&pth[0], NULL, &network, NULL);
    pthread_create(&pth[1], NULL, &clients_ping, NULL);
    pthread_create(&pth[2], NULL, &terminal, NULL);

    sigwait(&set, &sig);
    for (i = 0; i<3; i++) pthread_cancel(pth[i]);
    exit(EXIT_SUCCESS);
}