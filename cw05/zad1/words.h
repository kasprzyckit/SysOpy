#ifndef WORDSLIST
#define WORDSLIST

#define MAX_WRD 50
#define MAX_ARG 100

typedef struct words_list
{
    char *list[MAX_WRD];
    size_t length;
} words_list;

typedef struct exp_list
{
    words_list wlist[MAX_ARG];
    size_t length;
} exp_list;

words_list getwords(const char *text);
exp_list tokenize(const char *text);

#endif