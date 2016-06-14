#include <stdio.h>
#include <stdlib.h>

#include "tokeniser.h"
#include "stack.h"

//#define DEBUG_EXPR_STACK

void push(struct stack *s, struct token *t) {
#ifdef DEBUG_EXPR_STACK
	printf("push(%p) %d: ", s, s->sp);
	printf("<%d: %s>", t->id, sym_from_id(t->id));
	printf("\n");
#endif

	s->t[s->sp++] = t;
	if (s->sp >= MAX_EXPR_STACK) {
		printf("expression stack overflow\n");
		exit(1);
	}
}

struct token *pop(struct stack *s) {

	if(--s->sp < 0) {
		printf("expression stack underflow\n");
		exit(1);
	}

#ifdef DEBUG_EXPR_STACK
	do {
	struct token *t;
	t = s->t[s->sp];
	printf("pop (%p) %d: ", s, s->sp);
	printf("<%d: %s>", t->id, sym_from_id(t->id));
	printf("\n");
	} while (0);
#endif

	return s->t[s->sp];
}

struct token *pop_nocheck(struct stack *s) {

	if(--s->sp >= 0)
		return s->t[s->sp];

	return NULL;
}

struct token *peek(struct stack *s) {
	if(s->sp > 0)
		return s->t[s->sp-1];

	return NULL;
}

