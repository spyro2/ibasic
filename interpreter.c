#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "expression.h"
#include "interpreter.h"


#define IBASIC_STACK_SIZE 128

static struct value ibasic_stack[IBASIC_STACK_SIZE];
struct value *ibasic_stack_p = &ibasic_stack[0];

/* Allocate storage for variables */
struct value *val_alloc(char *name) {
	struct value *v;

	if(ibasic_stack_p - &ibasic_stack[0] == IBASIC_STACK_SIZE) {
		printf("Stack overflow!\n");
		exit(1);
		return NULL;
	}

	v = ibasic_stack_p++;
	if(name) {
		v->name = malloc(strlen(name)+1);
		strcpy(v->name, name);
	}

	return v;
}

//static inline void val_push(struct value *v) {
//	memcpy(ibasic_stack_p, v, sizeof(*v));
//	ibasic_stack_p++;
//}

/* Remove a variable / value from the stack. */
struct value *val_pop(void) {
	struct value *v = --ibasic_stack_p;

	if(v->name) {
		free(v->name);
		v->name = NULL;
	}

	return v;
}

/* Descend the stack, looking for a variable with a matching name. */
/* Caching might help in future */

struct value *lookup_var(char *name) {
	struct value *var = ibasic_stack_p-1;

	while(var >= &ibasic_stack[0]) {
		if(var->name && !strcmp(var->name, name))
			return var;
		var--;
	}

	return NULL;
}


#define call_eval(v, e) \
	do { \
	v = eval(e); \
	if(!v) \
		v = val_pop(); \
	} while(0)

static struct value *interpret_assign(struct ast_entry *e) {
	struct value *v;
	struct value *l;

	/* lookup_var */
	l = lookup_var(e->val->data.s);

	if(!l)
		l = val_alloc(e->val->data.s);

	call_eval(v, e->next->child);

	l->type = v->type;
	l->data.i = v->data.i;

	return l;
}

static int interpret_print(struct ast_entry *n) {

	do {
		if(n->id == ast_expression) {
			struct value *v;

			call_eval(v, n->child);

			switch(v->type) {
				case type_int:    printf("%d", v->data.i);break;
				case type_float:  printf("%f", v->data.d);break;
				case type_string: printf("%s", v->data.s);break;
				default:
					printf("Unknown type!\n");
					exit(1);
			}

		}
		n = n->next;
	} while(n);

	printf("\n");

	return 0;
}

static int interpret_condition(struct ast_entry *e) {
	struct value *a, *b;

	call_eval(a, e->child->child);
	call_eval(b, e->child->next->child);

	switch (e->id) {
	case tokn_eq:
		if(a->data.i == b->data.i) goto out_true; break;
	case tokn_ne:
		if(a->data.i != b->data.i) goto out_true; break;
	case tokn_gt:
		if(a->data.i > b->data.i) goto out_true; break;
	case tokn_lt:
		if(a->data.i < b->data.i) goto out_true; break;
	case tokn_ge:
		if(a->data.i >= b->data.i) goto out_true; break;
	case tokn_le:
		if(a->data.i <= b->data.i) goto out_true; break;
	default:
		printf("Unsupported condition code\n");
		exit(1);
	}

	return 0;

out_true:
	return 1;
}

static int interpret_statement(struct ast_entry *e, struct value *ret) {
	struct ast_entry *n = e->child;
	int r;

//	printf("AST entry %d (%s)\n", e->id, sym_from_id(e->id)?sym_from_id(e->id)->name:"");

	switch (e->id) {
		case tokn_repeat:
			do {
				r = interpret_block(n, NULL);
				if(r)
					return r;
			} while (!interpret_condition(n->next));
			break;
		case tokn_while:
			while (interpret_condition(n)) {
				r = interpret_block(n->next, NULL);
				if(r)
					return r;
			}
			break;
		case tokn_if:
			if(interpret_condition(n))
				interpret_block(n->next, NULL);
			else
				interpret_block(n->next->next, NULL);
			break;
		case tokn_print:
			interpret_print(n);
			break;
		case tokn_assign:
			interpret_assign(n);
			break;
		case tokn_for:
			{
				struct value *l, *e;
				int t, s = 1;

				l = interpret_assign(n->child);

				n = n->next;
				call_eval(e, n->child);

				t = e->data.i;

				n = n->next;
				if(n->id == ast_expression) {
					call_eval(e, n->child);
					s = e->data.i;

					n = n->next;
				}

				if(s > 0) {
					for(; l->data.i <= t; l->data.i += s)
						interpret_block(n, NULL);
				}
				else {
					for(; l->data.i >= t; l->data.i += s)
						interpret_block(n, NULL);
				}
			}
			break;
		case tokn_end:
			exit(0);
		case ast_expression: //FIXME: this is a bit iffy.
			{
				struct value *e;
				call_eval(e, n);

				ret->type = e->type;
				ret->data.i = e->data.i;
			}
			break;
		default:
			printf("Unexpected AST entry %d (%s)\n", e->id, sym_from_id(e->id)?sym_from_id(e->id)->name:"");
			exit(1);
	}

	return 0;
}

int interpret_block(struct ast_entry *e, struct value *ret) {
	struct ast_entry *n = e->child;
	int r;

	do {
		r = interpret_statement(n, ret);

		if(r)
			return r;

		n = n->next;
	} while(n);

	return 0;
}

void interpret(struct ast_entry *e) {
	struct ast_entry *n = e->child;

	do {
		if(n->id == ast_block)
			interpret_block(n, NULL);
		else {
			printf("invalid AST entry\n");
			exit(1);
		}

		n=n->next;
	} while (n);
}
