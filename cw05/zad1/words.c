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

exp_list tokenize(const char *text)
{
    words_list wl = getwords(text);
    exp_list el;
    int i, j = 0, k = 0, r = 0;
    for (i = 0; i < wl.length; i++)
    {
        if (strcmp(wl.list[i], "|") == 0)
        {
            if (i > r)
            {
                el.wlist[j].length = i - r;
                el.wlist[j].list[k] = NULL;
                k = 0;
                j++;
            }
            r = i + 1;
            continue;
        }
        el.wlist[j].list[k] = wl.list[i];
        k++;
    }
    if (i > r)
   	{
   		el.wlist[j].length = i - r;
   		el.wlist[j].list[k] = NULL;
   		j++;
   	}
    el.length = j;

    return el;
}
