#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "words.h"

words_list getwords(const char *text)
{
    int i, count = 0, index_start_word = 0, new_word = 0;
    size_t text_len = strlen(text);
    words_list wl;

    for (i = 0; i <= text_len; ++i)
    {
        if (isspace(text[i]) || i == text_len)
        {
            if (new_word)
            {
                wl.list[count] = strndup(text + index_start_word, i - index_start_word);
                new_word = 0;
                count++;
            }
        }
        else if (! new_word)
        {
            new_word = 1;
            index_start_word = i;
        }
    }
    wl.list[count] = NULL;
    wl.length = count;

    return wl;
}