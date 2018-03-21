#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stack.h"

stack_s* init_stack()
{
	stack_s* s = (stack_s*) calloc(1, sizeof(stack_s));
	s->top = 0;
	return s;
}

void push(stack_s* s, char* el, int size)
{
	if (s == NULL) return;
	if (s->top == STACK_MAX || size >= STACK_WIDTH-1) return;
	s->elems[s->top][size+1] = '\0';
	strcpy(s->elems[s->top], el);
	s->sizes[s->top] = size;
	s->top++;
}

void pop(stack_s* s, char* el)
{
	if (s == NULL) return;
	if (s->top == 0) return;
	s->top--;
	strcpy(el, s->elems[s->top]);
	el[s->sizes[s->top]] = '\0';
}

void delete_stack(stack_s* s)
{
	if (s == NULL) return;
	free(s);
}
int is_empty(stack_s* s)
{
	if (s == NULL) return 0;
	return (s->top == 0);
}
int is_full(stack_s* s)
{
	if (s == NULL) return 0;
	return (s->top == STACK_MAX);
}
