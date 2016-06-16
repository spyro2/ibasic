#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "expression.h"
#include "interpreter.h"


#define IBASIC_STACK_SIZE 128

struct ibasic_state {
	struct value *stack;
	struct value *stack_p;
	struct value *esp;

	struct ast_entry *next;
};

static struct ibasic_state state;

/* Allocate storage on the stack. */
struct value *stack_alloc(char *name) {
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

/* Emit a stack frame marker */
struct value *stack_alloc_frame(void) {
	struct value *v;

	state.esp = state.stack_p;
	v = stack_alloc(NULL);
	v->type = type_frame;

	return v;
}

/* Remove a variable / value from the stack. */
struct value *stack_pop(void) {
	struct value *v = --state.stack_p;

	if(v->name) {
		free(v->name);
		v->name = NULL;
	}

	return v;
}

/* Mark the current stack frame as active */
void stack_set_frame(struct value *v) {
	state.esp = NULL;
}

void stack_unwind_frame(void) {
	struct value *v;

	while(state.stack_p > state.stack) {
		v = stack_pop();
		if(v->type == type_frame) {
			/* Special rules apply when unwinding the stack.
			 * stack frame entries must have their type reset
			 * or unwind will break if an entry is reused but not
			 * initialised in future.
			 */
			v->type = 0;
			break;
		}
		else if(v->type == type_a_int) {
			free(v->data.ip);
		}
	}
}

/* Descend the stack, looking for a variable with a matching name.
 *
 * If the effective stack pointer is non-NULL, start from that
 * position in the stack. (used during function call setup to copy variables
 * from the calling functions stack frame).
 *
 * If the variable is not found in the top stack frame, attempt to find it
 * in the bottom one.
 *
 * Caching might help in future
 */
struct value *stack_lookup_var(char *name) {
	struct value *var = (state.esp ? state.esp : state.stack_p) - 1;

	while(var >= state.stack && var->type != type_frame) {
		if(var->name && !strcmp(var->name, name))
			return var;
		var--;
	}

	var = state.stack;
	while(var < state.stack_p && var->type != type_frame) {
		if(var->name && !strcmp(var->name, name))
			return var;
		var++;
	}

	return NULL;
}


#define call_eval(v, e) \
	do { \
	v = eval(e); \
	if(!v) \
		v = stack_pop(); \
	} while(0)

static inline struct value *interpret_assign(struct ast_entry *n) {
	struct ast_entry *e = n->child;
	struct value *v, *l;
	int idx;

	l = stack_lookup_var(e->val->data.s);

	if(e->id == tokn_array) {
		call_eval(v, e->child);
		if(v->type != type_int) {
			printf("Array index must be an integer!\n");
			exit(1);
		}
		idx = v->data.i;
	}

	call_eval(v, e->next);

	if(!l) {
		if(e->id == tokn_label) {
			l = stack_alloc(e->val->data.s);
			l->type = v->type;
		}
		else if (e->id == tokn_array) {
			printf("Array undimensioned!\n");
			exit(1);
		}
	}

	switch(l->type) {
		case type_int:
			l->data.i = v->data.i;
		break;
		case type_a_int:
			l->data.ip[idx] = v->data.i;
		break;
		default:
			printf("Unhandled type\n");
	}

	return l;
}

static inline struct value *interpret_dim(struct ast_entry *n) {
	struct ast_entry *e = n->child;
	struct value *v, *l;

	l = stack_lookup_var(e->val->data.s);

	if(l) {
		printf("%s Already exists!\n", e->val->data.s);
		exit(1);
	}

	l = stack_alloc(e->val->data.s);

	call_eval(v, e->child);

	l->type = type_a_int; // FIXME: types!!! v->type;
	l->data.ip = malloc(v->data.i * sizeof(int));
	l->size = v->data.i * sizeof(int);

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
					printf("Unhandled type (%d)!\n", v->type);
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
//		printf("AST entry %d (%s)\n", n->id, sym_from_id(n->id));

		switch (n->id) {
			/* Declaration / allocation */
			case tokn_dim:
				interpret_dim(n);
				break;

			/* Assignment */
			case tokn_assign:
				interpret_assign(n);
				break;

			/* IO */
			case tokn_print:
				interpret_print(n);
				break;

			/* Loops */
			case tokn_repeat:
				do {
					r = interpret_block(e, NULL);
					if(r == RET_BREAK || r == RET_END)
						break;
				} while (!interpret_condition(e->next));

				if(r == RET_BREAK || r == RET_CONTINUE)
					r = RET_OK;

				break;
			case tokn_while:
				while (interpret_condition(e)) {
					r = interpret_block(e->next, NULL);
					if(r == RET_BREAK || r == RET_END)
						break;
				}

				if(r == RET_BREAK || r == RET_CONTINUE)
					r = RET_OK;

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
							if(r == RET_BREAK || r == RET_END)
								break;
						}
					}
					else {
						for(; l->data.i >= t; l->data.i += s) {
							r = interpret_block(e, NULL);
							if(r == RET_BREAK || r == RET_END)
								break;
						}
					}
				}

				if(r == RET_BREAK || r == RET_CONTINUE)
					r = RET_OK;

				break;

			/* Function calling and return */
			case tokn_fn:
			case tokn_proc:
				call_proc_or_fn(n, NULL);
				break;
			case ast_expression:
				{
					struct value *v;

					call_eval(v, n);

					if(ret) {
						ret->type = v->type;
						ret->data.i = v->data.i;
					}
				}
				return RET_OK;

			/* Flow control / conditionals */
			case tokn_if:
				if(interpret_condition(e))
					r = interpret_block(e->next, NULL);
				else
					if(e->next->next)
						r = interpret_block(e->next->next, NULL);
				break;
			case tokn_case:
				{
					struct value *v;
					int c;

					call_eval(v, e);

					c = v->data.i;

					e = e->next;

					do {
						int d = 1;

						if(e->id == tokn_when) {
							struct ast_entry *ec = e->child;

							d = 0;

							do {
								struct value *w;

								call_eval(w, ec);

								if(c == w->data.i)
									d = 1;

								ec = ec->next;

							} while(!d && ec->id != ast_block);
						}

						if(d) {
							r = interpret_block(e->last_child, NULL);
							break;
						}

						e = e->next;

					} while (e && r != RET_END);
				}

				break;
			case tokn_break:
				return RET_BREAK;
			case tokn_continue:
				return RET_CONTINUE;
			case tokn_end:
				return RET_END;
			default:
				printf("Unexpected AST entry %d (%s)\n", n->id, sym_from_id(n->id));
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
		stack_unwind_frame();
	}

	else {
		printf("invalid AST entry\n");
		exit(1);
	}

	free(state.stack);

}
