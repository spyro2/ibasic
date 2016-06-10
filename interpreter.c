#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "expression.h"
#include "interpreter.h"


#define IBASIC_STACK_SIZE 128

struct ibasic_state {
	struct value *stack;
	struct value *stack_p;
	struct value *global_frame;

	struct ast_entry *next;
};

static struct ibasic_state state;

/* Allocate storage for variables */
struct value *val_alloc(char *name) {
	struct value *v;

	if(state.stack_p - state.stack == IBASIC_STACK_SIZE) {
		printf("Stack overflow!\n");
		exit(1);
		return NULL;
	}

	v = state.stack_p++;
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
	struct value *v = --state.stack_p;

	if(v->name) {
		free(v->name);
		v->name = NULL;
	}

	return v;
}

/* Emit a frame marker */
void val_alloc_frame(void) {
	struct value *v = val_alloc(NULL);
	v->type = type_frame;

	if(!state.global_frame)
		state.global_frame = v;
}

void unwind_frame(void) {
	struct value *v;
	while(state.stack_p > state.stack) {
		v = val_pop();
		if(v->type == type_frame) {
			if(v == state.global_frame)
				state.global_frame = NULL;
			break;
		}
	}
}

/* Descend the stack, looking for a variable with a matching name. */
/* Caching might help in future */

struct value *lookup_var(char *name) {
	struct value *var = state.stack_p-1;

	while(var >= state.stack) {
		if(var->type == type_frame)
			break;
		if(var->name && !strcmp(var->name, name))
			return var;
		var--;
	}

	var = state.global_frame;
	while(var >= state.stack) {
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

static inline struct value *interpret_assign(struct ast_entry *n) {
	struct ast_entry *e = n->child;
	struct value *v;
	struct value *l;

	/* lookup_var */
	l = lookup_var(e->val->data.s);

	if(!l)
		l = val_alloc(e->val->data.s);

	call_eval(v, e->next);

	l->type = v->type;
	l->data.i = v->data.i;

	return l;
}

static inline int interpret_print(struct ast_entry *n) {
	struct ast_entry *e = n->child;

	do {
		if(e->id == ast_expression) {
			struct value *v;

			call_eval(v, e);

			switch(v->type) {
				case type_int:    printf("%d", v->data.i);break;
				case type_float:  printf("%f", v->data.d);break;
				case type_string: printf("%s", v->data.s);break;
				default:
					printf("Unknown type!\n");
					exit(1);
			}

		}
		e = e->next;
	} while(e);

	printf("\n");

	return 0;
}

static inline int interpret_condition(struct ast_entry *n) {
	struct ast_entry *e = n->child;
	struct value *a, *b;

	call_eval(a, e);
	call_eval(b, e->next);

	switch (n->id) {
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

#define RET_OK 0
#define RET_BREAK 1
#define RET_CONTINUE 2
#define RET_END 4

int interpret_block(struct ast_entry *b, struct value *ret) {
	struct ast_entry *n = b->child;
	int r = RET_OK;


	do {
		struct ast_entry *e = n->child;
//		printf("AST entry %d (%s)\n", n->id, sym_from_id(n->id)?sym_from_id(n->id)->name:"");

		switch (n->id) {
			case tokn_repeat:
				do {
					r = interpret_block(e, NULL);
					if(r == RET_BREAK)
						break;
				} while (!interpret_condition(e->next));

				if(r == RET_BREAK || r == RET_CONTINUE)
					r = RET_OK;

				break;
			case tokn_while:
				while (interpret_condition(e)) {
					r = interpret_block(e->next, NULL);
					if(r == RET_BREAK)
						break;
				}

				if(r == RET_BREAK || r == RET_CONTINUE)
					r = RET_OK;

				break;
			case tokn_if:
				if(interpret_condition(e))
					r = interpret_block(e->next, NULL);
				else
					if(e->next->next)
						r = interpret_block(e->next->next, NULL);
				break;
			case tokn_print:
				interpret_print(n);
				break;
			case tokn_assign:
				interpret_assign(n);
				break;
			case tokn_for:
				{
					struct value *l, *v;
					int t, s = 1;

					l = interpret_assign(e);

					e = e->next;
					call_eval(v, e);

					t = v->data.i;

					e = e->next;
					if(e->id == ast_expression) {
						call_eval(v, e);
						s = v->data.i;

						e = e->next;
					}

					if(s > 0) {
						for(; l->data.i <= t; l->data.i += s) {
							r = interpret_block(e, NULL);
							if(r == RET_BREAK)
								break;
						}
					}
					else {
						for(; l->data.i >= t; l->data.i += s) {
							r = interpret_block(e, NULL);
							if(r == RET_BREAK)
								break;
						}
					}
				}

				if(r == RET_BREAK || r == RET_CONTINUE)
					r = RET_OK;

				break;
			case tokn_fn:
			case tokn_proc:
				call_proc_or_fn(n, NULL);
				break;
			case tokn_end:
				return RET_END;
			case ast_expression: //FIXME: this is a bit iffy.
				{
					struct value *v;

					call_eval(v, n);

					if(ret) {
						ret->type = v->type;
						ret->data.i = v->data.i;
					}
				}
				break;
			case tokn_break:
				return RET_BREAK;
			case tokn_continue:
				return RET_CONTINUE;
			default:
				printf("Unexpected AST entry %d (%s)\n", n->id, sym_from_id(n->id)?sym_from_id(n->id)->name:"");
				exit(1);
		}

		n = n->next;
	} while(!r && n);

	return r;
}

void interpret(struct ast_entry *e) {
	state.stack = calloc(IBASIC_STACK_SIZE, sizeof(*state.stack));
	state.stack_p = state.stack;

	if(e->id == ast_block) {
		interpret_block(e, NULL);
		unwind_frame();
	}

	else {
		printf("invalid AST entry\n");
		exit(1);
	}

	free(state.stack);

}
