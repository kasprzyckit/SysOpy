#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>

#define SEQUAL      1
#define SLESS       2
#define SGREATER    3

#define PALL        1
#define PCONS       0

#define ANSI_RED     "\x1b[91m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_MAGNETA "\x1b[35m"
#define ANSI_RESET   "\x1b[0m"

int prs_s, cons_s, buff_size, l, search_mode, print_mode, nk;
FILE* buff_src;

char** buffer = NULL;
int write_ind = 0, read_ind = 0, count = 0;

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t full_cond = PTHREAD_COND_INITIALIZER;

void sigexit(int sig) {exit(EXIT_FAILURE);};
void err(const char* msg);
void load_configuration(char* const filepath);
void __exit();
void consumer_out(char* buff, int ind);

void* producent(void* arg)
{
    char* buff = NULL;
    size_t n = 0;
    while (1)
    {
        pthread_mutex_lock(&count_mutex);
        while (count >= buff_size)
            pthread_cond_wait(&full_cond, &count_mutex);
        if (getline(&buff, &n, buff_src) <= 0)
        {
            pthread_mutex_unlock(&count_mutex);
            break;
        }
        if (print_mode == PALL) printf(ANSI_MAGNETA"Producer puts a line into "ANSI_BLUE"%i "ANSI_GREEN"\t%i / %i\n"ANSI_RESET, write_ind, count+1, buff_size);
        buffer[write_ind] = buff;
        write_ind = (write_ind+1)%buff_size;
        count++;

        pthread_cond_signal(&empty_cond);
        pthread_mutex_unlock(&count_mutex);

        n = 0;
        buff = NULL;
    }
    pthread_exit((void*) 0);
}

void* consumer(void* arg)
{
    char* buff;
    while (1)
    {
        pthread_mutex_lock(&count_mutex);
        while(count <= 0)
            pthread_cond_wait(&empty_cond, &count_mutex);

        buff = buffer[read_ind];
        buffer[read_ind] = NULL;

        if (print_mode == PALL) printf(ANSI_CYAN"Consument reads a line from "ANSI_BLUE"%i "ANSI_GREEN"\t%i / %i\n"ANSI_RESET, read_ind, count-1, buff_size);
        consumer_out(buff, read_ind);

        read_ind = (read_ind+1)%buff_size;
        count--;

        pthread_cond_signal(&full_cond);
        pthread_mutex_unlock(&count_mutex);

        free(buff);
    }
    pthread_exit((void*) 0);
}

int main(int argc, char *argv[])
{
    if (argc < 2) err("Too few arguments!");
    
    load_configuration(argv[1]);
    atexit(__exit);
    signal(SIGINT, sigexit);

    pthread_t* prods = malloc(prs_s * sizeof(pthread_t));
    pthread_t* cons = malloc(cons_s * sizeof(pthread_t));
    int i;

   buffer = malloc(buff_size * sizeof(char*));

    for (i = 0; i<prs_s; i++) if (pthread_create(&prods[i], NULL, &producent, NULL)) err("Producent thread");
    for (i = 0; i<cons_s; i++) if (pthread_create(&cons[i], NULL, &consumer, NULL)) err("Consumer thread");

    if (nk) alarm(nk);
    for (i = 0; i<prs_s; i++) if (pthread_join(prods[i], NULL)) err("Thread prod join");
    while (1)
    {
        pthread_mutex_lock(&count_mutex);
        if (count == 0) break;
        pthread_mutex_unlock(&count_mutex);
    }
    exit(EXIT_SUCCESS);
}

void consumer_out(char* buff, int ind)
{
    int flag;
    if (buff[strlen(buff)-1] == '\n') buff[strlen(buff)-1] = '\0';
    switch (search_mode)
    {
        case SEQUAL:    flag = (strlen(buff) == l); break;
        case SGREATER:  flag = (strlen(buff) > l);  break;
        case SLESS:     flag = (strlen(buff) < l);  break;
    }
    if (flag) printf(ANSI_BLUE"%i: "ANSI_RESET"%s\n", ind, buff);
}

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf(ANSI_RED"%s\n"ANSI_RESET, msg);
    exit(EXIT_FAILURE);
}

void __exit()
{
    if (buffer) free(buffer);
    if (buff_src) fclose(buff_src);
    pthread_mutex_destroy(&count_mutex);
    pthread_cond_destroy(&empty_cond);
    pthread_cond_destroy(&full_cond);
}

void load_configuration(char* const filepath)
{
    FILE* conf;
    if ((conf = fopen(filepath, "r")) < 0) err("Conf file open");
    char buff[1024];
    fread(buff, 1024, 1, conf);
    
    char *p = strtok(buff, " ");
    prs_s = atoi(p);
    p = strtok (NULL, " ");
    cons_s = atoi(p);
    p = strtok (NULL, " ");
    buff_size = atoi(p);
    p = strtok (NULL, " ");
    if ((buff_src = fopen(p, "r")) == NULL) err("Source file");
    p = strtok (NULL, " ");
    l = atoi(p);
    p = strtok (NULL, " ");
    search_mode = (strcmp(p, "LT") == 0) ? SLESS : ((strcmp(p, "GT") == 0) ? SGREATER : SEQUAL);
    p = strtok (NULL, " ");
    print_mode = (strcmp(p, "ALL") == 0) ? PALL : PCONS;
    p = strtok (NULL, " ");
    nk = atoi(p);
    if (prs_s <= 0 || cons_s <= 0 || l < 0) err("Incorrecr conf");

    if(fclose(conf) < 0) perror("Conf file close");
}