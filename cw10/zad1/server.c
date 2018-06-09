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
#include "../calc.h"

#define ANSI_RED     "\x1b[91m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_MAGNETA "\x1b[35m"
#define ANSI_RESET   "\x1b[0m"

#define MAX_EVENTS      40
#define MAX_CLIENTS     20

int port_tcp;
char socket_path[UNIX_PATH_MAX];
int inet_socket, unix_socket;
client_list clist;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

void* terminal(void *arg);
void* network(void *arg);
void* clients_ping(void *arg);
void init_socket(void);
void parse_args(int argc, char const *argv[]);
void __exit(void);
void err(const char* msg);

int setnoblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;
    flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}

int send_packet(int client_fd, msg_t msg_type, char* msg)
{
    void *buff;
    packet_t pck;
    pck.msg_type = msg_type;
    pck.msg_length = strlen(msg) + 1;

    buff = malloc(sizeof(packet_t) + pck.msg_length);
    memcpy(buff, &pck, sizeof(packet_t));
    memcpy(buff+sizeof(packet_t), msg, pck.msg_length);
    if (write(client_fd, buff, 
        sizeof(packet_t) + pck.msg_length) < 0) return -1;
    return 0;
}

void* terminal(void *arg)
{
    char* line = NULL;
    size_t n = 0;
    int count;
        printf("> ");
    for (;;)
    {
        count = getline(&line, &n, stdin);
        line[count-1] = '\0';

        pthread_mutex_lock(&list_mutex);
        send_packet(get_next_fd(&clist), SERVER_CALC, line);
        pthread_mutex_unlock(&list_mutex);

        free(line);
        n = 0;
    }

    return (void*) 0;
}

void close_socket(int socket_fd)
{
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
}

int accept_client(int server_fd)
{
    int client_fd;
    packet_t pck;
    char buff[MAX_MSG];

    if ((client_fd = accept(server_fd, NULL, NULL)) < 0) return 0;
    read(client_fd, &pck, sizeof(packet_t));
    read(client_fd, buff, pck.msg_length);

    pthread_mutex_lock(&list_mutex);

    if (clist.size >= MAX_CLIENTS)
    {
        pthread_mutex_unlock(&list_mutex);

        send_packet(client_fd, SERVER_REJECT, "Server full");
        close_socket(client_fd);
        return 0;
    } 
    if (is_present_clist(&clist, buff))
    {
        pthread_mutex_unlock(&list_mutex);

        send_packet(client_fd, SERVER_REJECT, "Name already in use");
        close_socket(client_fd);
        return 0;
    }

    add_clist(&clist, client_fd, buff);

    pthread_mutex_unlock(&list_mutex);

    if(send_packet(client_fd, SERVER_ACCEPT, "CALC cluster server: welcome!")<0)
        return 0;
    return client_fd;
}

void accept_message(int client_fd)
{
    packet_t pck;
    char buff[MAX_MSG];
    int t;
    if ((t = read(client_fd, &pck, sizeof(packet_t))) <= 0) return;
    if ((t = read(client_fd, buff, pck.msg_length)) <= 0) return;
    switch(pck.msg_type)
    {
        case CLIENT_PONG:
            pthread_mutex_lock(&list_mutex);

            confirm_ping(&clist, client_fd);

            pthread_mutex_unlock(&list_mutex);
            break;
        case CLIENT_ANSW:
            printf("%s\n> ", buff);
            fflush(stdout);
            break;
        case CLIENT_ERR:
            printf(ANSI_RED"%s"ANSI_RESET"\n> ", buff);
            fflush(stdout);
            break;
        case CLIENT_UNREG:
            pthread_mutex_lock(&list_mutex);

            close_socket(client_fd);
            remove_clist(&clist, client_fd);

            pthread_mutex_unlock(&list_mutex);
            break;
        default:
            break;
    }
}

void* network(void *arg)
{
    struct epoll_event ev, events[MAX_EVENTS];
    int epoll_fd, nfds, i, connection_fd;

    if ((epoll_fd = epoll_create1(0))< 0) err("Epoll creation");
    ev.events = EPOLLIN;
    ev.data.fd = inet_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, inet_socket, &ev) < 0)
        err("Epoll server add");
    ev.data.fd = unix_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, unix_socket, &ev) < 0)
        err("Epoll server add");
   ev.events = EPOLLIN | EPOLLET;

    for (;;)
    {
        if ((nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1)) < 0)
            err("Epoll wait");

        for (i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == inet_socket 
                || events[i].data.fd == unix_socket)
            {
                connection_fd = accept_client(events[i].data.fd);
                if (connection_fd)
                {
                    ev.data.fd = connection_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection_fd, &ev) 
                        < 0) printf(ANSI_RED"Epoll client error\n"ANSI_RESET);
                }
            }
            else accept_message(events[i].data.fd);
        }
    }
    return (void*) 0;
}

void* clients_ping(void *arg)
{
    struct client_node* p;
    for (;;)
    {
        pthread_mutex_lock(&list_mutex);

        for (p = clist.first; p != NULL; p = p->next)
        {
            if (p->ping == 0)
            {
                close_socket(p->fd);
                remove_clist(&clist, p->fd);
            }
        }

        reset_ping(&clist);

        for (p = clist.first; p != NULL; p = p->next)
            send_packet(p->fd, SERVER_PING, "Ping");

        pthread_mutex_unlock(&list_mutex);

        sleep(2);
    }

    return (void*) 0;
}

int main(int argc, char const *argv[])
{
    parse_args(argc, argv);

    init_socket();
    atexit(__exit);

    sigset_t set;
    int sig, i;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    pthread_t pth[3];
    pthread_create(&pth[0], NULL, &terminal, NULL);
    pthread_create(&pth[1], NULL, &network, NULL);
    pthread_create(&pth[2], NULL, &clients_ping, NULL);

    sigwait(&set, &sig);
    for (i = 0; i<3; i++) pthread_cancel(pth[i]);
    exit(EXIT_SUCCESS);
}

void init_socket(void)
{
    struct sockaddr_in addr_inet;
    struct sockaddr_un addr_unix;
    int one = 1;

    inet_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(inet_socket, SOL_SOCKET, SO_REUSEADDR, 
        &one, sizeof(int)) < 0) err("inet setsockopt");
    memset(&addr_inet, 0, sizeof(struct sockaddr_in));
    addr_inet.sin_family = AF_INET;
    addr_inet.sin_port = htons((uint16_t)port_tcp);
    addr_inet.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(inet_socket, (const struct sockaddr *) &addr_inet, 
        sizeof(struct sockaddr_in)) < 0) err("Inet socket bind");
    if (listen(inet_socket, MAX_CLIENTS) < 0) err("Inet socket listen");

    unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&addr_unix, 0, sizeof(struct sockaddr_un));
    addr_unix.sun_family = AF_UNIX;
    strncpy(addr_unix.sun_path, socket_path, sizeof(addr_unix.sun_path) - 1);
    if (bind(unix_socket, (const struct sockaddr *) &addr_unix, 
        sizeof(struct sockaddr_un)) < 0) err("Unix socket bind");
    if (listen(unix_socket, MAX_CLIENTS) < 0) err("Unix socket listen");

    init_clist(&clist);
}

void parse_args(int argc, char const *argv[])
{
    if (argc < 3) err("Too few arguments!");
    port_tcp = atoi(argv[1]);
    if (port_tcp == 0 && argv[1][0] != '0') err("Invalid TCP port!");
    sprintf(socket_path, "%s", argv[2]);
}

void __exit(void)
{
    close_socket(inet_socket);
    close_socket(unix_socket);

    struct client_node* p;
    for (p = clist.first; p != NULL; p = p->next) close_socket(p->fd);

    unlink(socket_path);

    pthread_mutex_destroy(&list_mutex);

    printf("\n");
}

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf(ANSI_RED"%s\n"ANSI_RESET, msg);
    exit(EXIT_FAILURE);
}