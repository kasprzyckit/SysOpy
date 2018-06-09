#include "calc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void init_clist(client_list* cl)
{
    cl->size = 0;
    cl->first = NULL;
    cl->counter = 0;
}

void add_clist(client_list* cl, int fd, char* name)
{
    struct client_node* tmp, *p = cl->first;
    if (p == NULL)
    {
        p = malloc(sizeof(struct client_node));
        p->fd = fd;
        p->name = malloc((strlen(name)+1)*sizeof(char));
        sprintf(p->name, "%s", name);
        p->next = NULL;
        p->ping = 1;
        cl->first = p;
        cl->size = 1;
    }
    else
    {
        while(p->next != NULL && p->next->fd < fd) p = p->next;
        tmp = p->next;
        p->next = malloc(sizeof(struct client_node));
        p->next->fd = fd;
        p->next->name = malloc((strlen(name)+1)*sizeof(char));
        sprintf(p->next->name, "%s", name);
        p->next->next = tmp;
        p->next->ping = 1;
        cl->size++;
    }
}

int remove_clist(client_list *cl, int fd)
{
    struct client_node* tmp, *p = cl->first;
    if (p == NULL) return 0;
    if (p->fd == fd)
    {
        cl->first = p->next;
        cl->size--;
        free(p->name);
        free(p);
        return 1;
    }
    while(p->next != NULL && p->next->fd != fd) p = p->next;
    if (p->next == NULL) return 0;
    tmp = p->next->next;
    free(p->next->name);
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

int get_next_fd(client_list *cl)
{
    struct client_node* p = cl->first;
    int i;
    if (cl->size == 0) return 0;
    for (i = 0; i<(cl->counter)%(cl->size); i++) p = p->next;
    cl->counter++;
    return p->fd;
}

void reset_ping(client_list *cl)
{
    struct client_node* p;
    for (p = cl->first; p != NULL; p = p->next) p->ping = 0;
}

int confirm_ping(client_list *cl, int fd)
{
    struct client_node* p = cl->first;
    while(p != NULL && p->fd != fd) p = p->next;
    if (p == NULL) return 0;
    p->ping = 1;
    return 0;
}