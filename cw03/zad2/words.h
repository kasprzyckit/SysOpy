#ifndef WORDSLIST
#define WORDSLIST

#define MAX_ARG 100

typedef struct words_list
{
    char *list[MAX_ARG];
    size_t length;
} words_list;

words_list getwords(const char *text);

#endif