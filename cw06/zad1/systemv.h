#ifndef __SYSTEMV
#define __SYSTEMV

#define UNDEF_MSG   0
#define MIRROR_MSG  1
#define CALC_MSG    2
#define TIME_MSG    3
#define END_MSG     4
#define INIT_MSG    5
#define STOP_MSG    6

#define MAX_MSG     100
#define MAX_CLIENTS 100

struct msgbuf 
{
    long mtype;
    char mtext[MAX_MSG];
};

#endif