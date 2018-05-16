#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <string.h>

#define  MAX( a, b ) ( ( a > b) ? a : b )
#define  MIN( a, b ) ( ( a < b) ? a : b )

#define ANSI_RED     "\x1b[91m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGNETA "\x1b[35m"
#define ANSI_RESET   "\x1b[0m"

FILE *inf, *outf, *filterf;
int thrs, wd, hg, c;
unsigned char** img = NULL;
float** filter = NULL;
unsigned char** out = NULL;

void parse_pgm();
void parse_filter();
void* filter_pgm(void *number);
void write_pgm(unsigned char** im, int w, int h);
void print_times(struct timespec* ts, struct timespec* te);
void err(const char* msg);
void __exit();

int main(int argc, char const *argv[])
{
    if (argc < 5) err("Too few arguments!");
    thrs = atoi(argv[1]);
    if (thrs <= 0) err("Incorrect number of threads!");

    atexit(__exit);

    if ((inf = fopen(argv[2], "r")) < 0) err("In file");
    if ((filterf = fopen(argv[3], "r")) < 0) err("filter");
    if ((outf = fopen(argv[4], "w")) < 0) err("Out file");
    
    parse_pgm();
    parse_filter();

    int i;
    out = (unsigned char**) malloc(hg * sizeof(unsigned char*));
    for (i = 0; i<hg; i++) out[i] = (unsigned char*) malloc(wd * sizeof(unsigned char));

    int *num;
    struct timespec tstart, tend;
    pthread_t* threads = (pthread_t*) malloc(thrs * sizeof(pthread_t));


    clock_gettime(CLOCK_MONOTONIC, &tstart);
    for (i = 0; i < thrs; i++)
    {
        num = (int*) malloc(sizeof(int));
        *num = i;
        if (pthread_create(&threads[i], NULL, &filter_pgm, (void*)num)) err("Thread");
    }
    for (i = 0; i<thrs; i++) if (pthread_join(threads[i], NULL)) err("Thread join");
    free(threads);
    clock_gettime(CLOCK_MONOTONIC, &tend);

    print_times(&tstart, &tend);
    write_pgm(out, wd, hg);

    exit(EXIT_SUCCESS);
}

void parse_pgm()
{
    char *buff;
    size_t n = 0;
    ssize_t count;
    int off = 0, i = 0, j = 0;

    getline(&buff, &n, inf); getline(&buff, &n, inf);
    wd = atoi(buff);
    while(! isspace(buff[off])) off++;
    hg = atoi(buff + off + 1);
    getline(&buff, &n, inf);

    img = (unsigned char**) malloc(hg * sizeof(unsigned char*));

    while ((count = getline(&buff, &n, inf)) > 0)
    {
        for (off = 0; off < count-1;)
        {
            if (j == 0) img[i] = (unsigned char*) malloc(wd * sizeof(unsigned char));
            img[i][j++] = (unsigned char) atoi(buff + off);
            if (j == wd) {j = 0; i++;}
            while (! isspace(buff[off]) && off < count) off++;
            while (isspace(buff[off]) && off < count) off++;
        }
        if (i == hg) break;
    }
    free(buff);
}

void parse_filter()
{
    char *buff;
    size_t n = 0;
    ssize_t count;
    int off = 0, i = 0, j = 0;

    getline(&buff, &n, filterf);
    c = atoi(buff);

    filter = (float**) malloc(c * sizeof(float*));

    while ((count = getline(&buff, &n, filterf)) > 0)
    {
        for (off = 0; off < count-1;)
        {
            if (j == 0) filter[i] = (float*) malloc(c * sizeof(float));
            filter[i][j++] = atof(buff + off);
            if (j == c) {j = 0; i++;}
            while (! isspace(buff[off]) && off < count) off++;
            while (isspace(buff[off]) && off < count) off++;
        }
    }

    free(buff);
}

void* filter_pgm(void *number)
{
    int i = hg*(*(int*)number) / thrs, j, k, l, tmpy, tmpx;
    int c2 = ceil(((float)c)/2.0), end = hg*(*(int*)number + 1) / thrs;
    float s;
    free(number);
    for (; i < end; i++)
    {
        tmpx = i - c2;
        for (j = 0; j < wd; j++)
        {
            s = 0;
            tmpy = j - c2;
            for (k = 0; k < c; k++) for (l = 0; l < c; l++)
                s += filter[k][l] * img[MIN(hg-1,MAX(0, tmpx+k))][MIN(wd-1, MAX(0, tmpy+l))];
            out[i][j] = round(s);
        }
    }
    pthread_exit((void*) 0);
}

void write_pgm(unsigned char** im, int w, int h)
{
    int i, j, k = 0;
    char buff[80];
    sprintf(buff, "P2\n%i %i\n255\n", w, h);
    fwrite(buff, 1, (size_t) strlen(buff), outf);
    for (i = 0; i<h; i++) for (j = 0; j<w; j++)
    {
        if (k == 0) buff[0] = '\0';
        sprintf(buff, "%s%i", buff, im[i][j]);
        if (k == 18)
        {
            sprintf(buff, "%s\n", buff);
            fwrite(buff, 1, (size_t) strlen(buff), outf);
            k = 0;
        }
        else
        {
            sprintf(buff, "%s ", buff);
            k++;
        }
    }
    if (k != 0)
    {
        sprintf(buff, "%s\n", buff);
        fwrite(buff, 1, (size_t) strlen(buff), outf);
    }
}

void print_times(struct timespec* ts, struct timespec* te)
{
    time_t s = te->tv_sec - ts->tv_sec;
    long n = te->tv_nsec - ts->tv_nsec;
    if (n < 0)
    {
        s--;
        n += 1000000000;
    }
    printf("Size of the image: " ANSI_BLUE"%i x %i\n"ANSI_RESET\
        "Size of the filter: " ANSI_BLUE"%i\n"ANSI_RESET\
        "Number of threads: " ANSI_BLUE"%i\n"ANSI_RESET\
        ANSI_MAGNETA"Time measured: %ld sec %ld nsec\n"ANSI_RESET, hg, wd, c, thrs, s, n);
}

void err(const char* msg)
{
    if (errno) perror(msg);
    else printf(ANSI_RED"%s\n"ANSI_RESET, msg);
    exit(EXIT_FAILURE);
}

void __exit()
{
    fclose(inf);
    fclose(filterf);
    fclose(outf);
    int i;
    if (img != NULL)
    {
        for(i=0;i<hg;i++) free(img[i]);
        free(img);
    }
    if (filter != NULL)
    {
        for(i=0;i<c;i++) free(filter[i]);
        free(filter);
    }
    if (out != NULL)
    {
        for(i=0;i<hg;i++) free(out[i]);
        free(out);
    }
}