#ifndef _CALC_SERVER
#define _CALC_SERVER

#define UNIX_PATH_MAX   108
#define MAX_MSG         100

typedef char msg_t;
#define CLIENT_REGISTER (char)0
#define CLIENT_PONG     (char)1
#define CLIENT_ANSW     (char)2
#define CLIENT_ERR      (char)3
#define CLIENT_UNREG    (char)4
#define SERVER_ACCEPT   (char)5
#define SERVER_REJECT   (char)6
#define SERVER_PING     (char)7
#define SERVER_CALC     (char)8

#include <sys/socket.h>

typedef struct {
    msg_t msg_type;
    int msg_length;
    int c_count;
    int id;
} packet_t;

struct client_node {
    struct sockaddr* address;
    socklen_t addr_size;
    char* name;
    int sock;
    int id;
    int ping;
    struct client_node* next;
} client_node;

typedef struct {
    int size;
    int counter;
    struct client_node* first;
} client_list;

void    init_clist(client_list* cl);
void    add_clist(client_list* cl, struct sockaddr* address, 
                    socklen_t addr_size, char* name, int id, int sock);
int     remove_clist(client_list *cl, int id);
int     is_present_clist(client_list *cl, char* name);
void    get_next_address(client_list *cl, struct sockaddr** address, 
                            socklen_t* addr_size, int* sock);
void    reset_ping(client_list *cl);
void    confirm_ping(client_list *cl, int id);

#endif