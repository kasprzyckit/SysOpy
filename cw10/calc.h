#ifndef _CALC_SERVER
#define _CALC_SERVER

#define UNIX_PATH_MAX   108
#define MAX_MSG         100

typedef char msg_t;
#define CLIENT_REGISTER (char)0
#define SERVER_ACCEPT   (char)1
#define SERVER_REJECT   (char)2
#define SERVER_PING     (char)3
#define CLIENT_PONG     (char)4
#define SERVER_CALC     (char)5
#define CLIENT_ANSW     (char)6
#define CLIENT_ERR      (char)7
#define CLIENT_UNREG    (char)8

typedef struct {
    msg_t msg_type;
    int msg_length;
    int c_count;
} packet_t;

struct client_node {
    int fd;
    char* name;
    int ping;
    struct client_node* next;
} client_node;

typedef struct {
    int size;
    int counter;
    struct client_node* first;
} client_list;

void    init_clist(client_list* cl);
void    add_clist(client_list* cl, int fd, char* name);
int     remove_clist(client_list *cl, int fd);
int     is_present_clist(client_list *cl, char* name);
int     get_next_fd(client_list *cl);
void    reset_ping(client_list *cl);
int     confirm_ping(client_list *cl, int fd);

#endif