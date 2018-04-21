#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include "systemv.h"

#define ANSI_RED     "\x1b[91m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_MAGNETA "\x1b[35m"
#define ANSI_RESET   "\x1b[0m"

int msgqueue;
void err(const char* msg);
void snd_err(const char* msgtext, int msgqid);
void sig_handler(int sig);
void set_sigint();
void register_client(int *clients, struct msgbuf* msg);
void remove_client(int *clients, struct msgbuf* msg);
void mirror_request(int client_queue_id, char* msg_text);
void calc_request(int clqid, char* msg, int len);
void time_request(int client_queue_id);
void __exit(void){msgctl(msgqueue, IPC_RMID, NULL);}

int main(int argc, char const *argv[])
{
    const char *rqsts[4] = {"MIRROR", "CALC", "TIME", "END"};
    int i, break_flag = 0;
    struct msgbuf msg;
    int clients[MAX_CLIENTS];
    for (i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;

    set_sigint();
    atexit(__exit);

    if ((msgqueue = msgget(ftok("./systemv.h", 0), IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO)) < 0) err("Server queue open");
    
    for (;;)
    {
        if (msgrcv(msgqueue, &msg, MAX_MSG, 0, MSG_NOERROR) < 0) err("Server receive");
        if (msg.mtype < 5) printf("Received "ANSI_BLUE"%s"ANSI_RESET" from "ANSI_GREEN \
            "#%i"ANSI_RESET"\n", rqsts[msg.mtype-1], msg.client_id);
        switch (msg.mtype)
        {
            case INIT_MSG:
                register_client(clients, &msg);
                break;
            case STOP_MSG:
                remove_client(clients, &msg);
                break;
            case MIRROR_MSG:
                mirror_request(clients[msg.client_id], msg.mtext);
                break;
            case CALC_MSG:
                calc_request(clients[msg.client_id], msg.mtext, strlen(msg.mtext));
                break;
            case TIME_MSG:
                time_request(clients[msg.client_id]);
                break;
            case END_MSG:
                break_flag = 1;
                break;
            
            default:
                printf(ANSI_RED"Unrecognized request:"ANSI_RESET" %s\n", msg.mtext);
                break;
        }
        if (break_flag) break;
    }

    printf("Exiting\n");
    exit(EXIT_SUCCESS);
}

void register_client(int *clients, struct msgbuf* msg)
{
    int i, tmp;
    tmp = atoi(msg->mtext);
    for (i = 0; i < MAX_CLIENTS && clients[i] != -1; i++) {;}
    if (i < MAX_CLIENTS)
    {
        clients[i] = tmp;
        msg->mtype = RPLY_MSG;
        sprintf(msg->mtext, "%i", i);
        if (msgsnd(clients[i], msg, MAX_MSG, 0) < 0)
        {
            perror("Connection init msg");
            clients[i] = -1;
            return;
        }
        printf("Client "ANSI_GREEN"#%i"ANSI_RESET" registred: " \
            ANSI_MAGNETA"%i"ANSI_RESET"\n", i, msg->client_pid);
    }
    else
    {
        msg->mtype = ERR_MSG;
        sprintf(msg->mtext, "Queue full");
        if (msgsnd(tmp, msg, MAX_MSG, 0)) perror("Error init msg");
    }
}

void remove_client(int *clients, struct msgbuf* msg)
{
    if (msg->client_id < 0 || msg->client_id > MAX_CLIENTS) return;
    printf("Client "ANSI_GREEN"#%i"ANSI_RESET" removed: "\
        ANSI_MAGNETA"%i"ANSI_RESET"\n", msg->client_id, msg->client_pid);
    clients[msg->client_id] = -1;
}

void mirror_request(int client_queue_id, char* msg_text)
{
    char *p1 = msg_text;
    char *p2 = msg_text + strlen(msg_text) - 1;

    while (p1 < p2) {
        char tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
    struct msgbuf msg;
    msg.mtype = RPLY_MSG;
    sprintf(msg.mtext, "%s", msg_text);
    if (msgsnd(client_queue_id, &msg, MAX_MSG, 0) < 0) perror("Server send");
}

void calc_request(int clqid, char* msg, int len)
{
    char opr, tmp[10];
    int c = 0, n1, n2, res;
    while (msg[c] > '0' && msg[c] <= '9' && c < len) c++;
    if (c == 0) {snd_err("Malformed message", clqid); return;}
    sprintf(tmp, "%.*s",(c > 10) ? c : 10, msg);
    n1 = atoi(tmp);
    while (msg[c] == ' ' && c < len) c++;
    if (c == len) {snd_err("Malformed message", clqid); return;}
    opr = msg[c++];
    while (msg[c] == ' ' && c < len) c++;
    if (c == len || msg[c] < '0' || msg[c] > '9') {snd_err("Malformed message", clqid); return;}
    sprintf(tmp, "%.*s", (len-c > 10) ? len-c : 10, msg + c);
    n2 = atoi(tmp);
    switch (opr)
    {
        case '+': res = n1 + n2; break;
        case '-': res = n1 - n2; break;
        case '/': if (n2) res = n1 / n2; else {snd_err("Arithmetic error", clqid); return;} break;
        case '*': res = n1 * n2; break;
        default: snd_err("Malformed message", clqid); return;
    }
    struct msgbuf msgs;
    msgs.mtype = RPLY_MSG;
    sprintf(msgs.mtext, "%i", res);
    if (msgsnd(clqid, &msgs, MAX_MSG, 0) < 0) perror("Server send");
}

void time_request(int client_queue_id)
{
    struct msgbuf msg;
    msg.mtype = RPLY_MSG;

    FILE *date = popen("date", "r");
    if (date == NULL || date < 0) perror("date");
    fgets(msg.mtext, MAX_MSG_TXT, date);
    pclose(date);
    sprintf(msg.mtext, "%.*s", (int)strlen(msg.mtext)-1, msg.mtext);
    if (msgsnd(client_queue_id, &msg, MAX_MSG, 0) < 0) perror("Server send");
}

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf(ANSI_RED"%s\n"ANSI_RESET, msg);
    exit(EXIT_FAILURE);
}

void snd_err(const char* msgtext, int msgqid)
{
    struct msgbuf msg;
    msg.mtype = ERR_MSG;
    sprintf(msg.mtext,ANSI_RED "%s" ANSI_RESET, msgtext);
    msgsnd(msgqid, &msg, MAX_MSG, 0);
}

void sig_handler(int sig)
{
    if (sig == SIGINT)
    {
        printf("\nShutting down\n");
        exit(EXIT_SUCCESS);
    }
    else if (sig == SIGSEGV) err("Segmentation fault");
}

void set_sigint()
{
    struct sigaction act;
    act.sa_handler = sig_handler;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGINT, &act, NULL) < -1) err("Signal");
    if (sigaction(SIGSEGV, &act, NULL) < -1) err("Signal");
}