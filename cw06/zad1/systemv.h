#ifndef __SYSTEMV
#define __SYSTEMV

#define MIRROR_MSG  1
#define CALC_MSG    2
#define TIME_MSG    3
#define END_MSG     4
#define INIT_MSG    5
#define STOP_MSG    6
#define RPLY_MSG    7
#define ERR_MSG     8
#define UNDEF_MSG   9

#define MAX_MSG_TXT 100
#define MAX_MSG     sizeof(struct msgbuf)-sizeof(long)
#define MAX_CLIENTS 2

struct msgbuf 
{
    long mtype;
    char mtext[MAX_MSG_TXT];
    int client_id;
    pid_t client_pid;
};

#endif
