#ifndef __POS
#define __POS

#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RED     "\x1b[91m"
#define ANSI_MAGNETA "\x1b[35m"
#define ANSI_RESET   "\x1b[0m"

#define MAX_CLIENTS 100

typedef struct
{
    pid_t id;
    int sem;
} msg_t;

typedef struct
{
    short is_asleep;
    int seats_free;
    pid_t served_customer;
    sem_t prv[MAX_CLIENTS];
    int top;
    msg_t fifo[MAX_CLIENTS];
    int top_q;
} barber_state;

#define FIFO_PATH "fifo"
#define BARBER_SHARED "/barber_so"
#define SEM_SHOP "/shop_sem"
#define SEM_CPRES "/cpres_sem"
#define SEM_BREAD "/bread_sem"
#define SEM_CREAD "/cread_sem"
#define SEM_CUT "/cut_sem"
#define SEM_SEAT "/seat_sem"
#define QUE_SEAT "/queue_sem"

#endif