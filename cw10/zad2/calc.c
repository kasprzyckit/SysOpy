#include "calc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void init_clist(client_list* cl)
{
    cl->size = 0;
    cl->first = NULL;
    cl->counter = 0;
}

void add_clist(client_list* cl, struct sockaddr* address, socklen_t addr_size, 
    char* name, int id, int sock)
{
    struct client_node* tmp, *p = cl->first;
    if (p == NULL)
    {
        p = malloc(sizeof(struct client_node));
        p->address = malloc(addr_size);
        memcpy(p->address, address, addr_size);
        p->addr_size = addr_size;
        p->name = malloc((strlen(name)+1)*sizeof(char));
        sprintf(p->name, "%s", name);
        p->id = id;
        p->sock = sock;

        p->next = NULL;
        p->ping = 1;
        cl->first = p;
        cl->size = 1;
    }
    else
    {
        while(p->next != NULL && p->next->id < id) p = p->next;
        tmp = p->next;
        p->next = malloc(sizeof(struct client_node));
        p->next->address = malloc(addr_size);
        memcpy(p->next->address, address, addr_size);
        p->next->addr_size = addr_size;
        p->next->name = malloc((strlen(name)+1)*sizeof(char));
        sprintf(p->next->name, "%s", name);
        p->next->id = id;
        p->next->sock = sock;
        p->next->next = tmp;
        p->next->ping = 1;
        cl->size++;
    }
}

int remove_clist(client_list *cl, int id)
{
    struct client_node* tmp, *p = cl->first;
    if (p == NULL) return 0;
    if (p->id == id)
    {
        cl->first = p->next;
        cl->size--;
        free(p->address);
        free(p->name);
        free(p);
        return 1;
    }
    while(p->next != NULL && p->next->id != id) p = p->next;
    if (p->next == NULL) return 0;
    tmp = p->next->next;
    free(p->next->name);
    free(p->next->address);
    free(p->next);
    p->next = tmp;
    cl->size--;
    return 1;
}

int is_present_clist(client_list *cl, char* name)
{
    struct client_node* p = cl->first;
    while(p != NULL && strcmp(p->name, name) != 0) p = p->next;
    return p != NULL;
}

void get_next_address(client_list *cl, struct sockaddr** address, 
    socklen_t* addr_size, int* sock)
{
    struct client_node* p = cl->first;
    int i;
    if (cl->size == 0) return;
    for (i = 0; i<(cl->counter)%(cl->size); i++) p = p->next;
    cl->counter++;
    *address = p->address;
    *addr_size = p->addr_size;
    *sock = p->sock;
    return;
}

void reset_ping(client_list *cl)
{
    struct client_node* p;
    for (p = cl->first; p != NULL; p = p->next) p->ping = 0;
}

void confirm_ping(client_list *cl, int id)
{
    struct client_node* p = cl->first;
    while(p != NULL && p->id != id) p = p->next;
    if (p == NULL) return;
    p->ping = 1;
    return;
}