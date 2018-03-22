#ifndef STACK
#define STACK

#define STACK_MAX 200
#define STACK_WIDTH 200

typedef struct stack_s
{
	char elems[STACK_MAX][STACK_WIDTH];
	int top;
	int sizes[STACK_MAX];
} stack_s;

stack_s* init_stack();
void push(stack_s* s, char* el, int size);
void pop(stack_s* s, char* el);
void delete_stack(stack_s* s);
int is_empty(stack_s* s);
int is_full(stack_s* s);
void clear_stack(stack_s* s);
#endif
