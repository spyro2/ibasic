#ifndef STACK_H_INCLUDED

#define MAX_EXPR_STACK 64
//#define DEBUG_EXPR_STACK

struct stack {
	struct token *t[MAX_EXPR_STACK];
	int sp;
};

void push(struct stack *s, struct token *t);
struct token *pop(struct stack *s);
struct token *pop_nocheck(struct stack *s);
struct token *peek(struct stack *s);

#define STACK_H_INCLUDED
#endif

