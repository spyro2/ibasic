#include <stdio.h>

#include "ast.h"
#include "expression.h"


static struct value *var[10];

static struct value *call_stack[10];
static struct value **call_stack_p = &call_stack[0];

void val_push(struct value *v) {
	*call_stack_p = v;
	call_stack_p++;
}

struct value *val_pop(void) {
	call_stack_p--;
	return *call_stack_p;
}

struct value *lookup_var(char *name) {
	int i = *name-'A';

	if(!var[i]) {
		var[i] = val_alloc();
		var[i]->type = type_int;
		val_get(var[i]);
	}

	return var[i];
}

static int interpret_block(struct ast_entry *e);

static int interpret_assign(struct ast_entry *e) {
	struct value *v;
	struct value *l;

	/* lookup_var */
	l = lookup_var(e->val->data.s);

	v = eval(e->next->child);

	l->data.i = v->data.i;

	val_put(v);

	return 0;
}

static int interpret_print(struct ast_entry *n) {

	do {
		if(n->id == ast_expression) {
			struct value *v = eval(n->child);

			switch(v->type) {
				case type_int:    printf("%d", v->data.i);break;
				case type_float:  printf("%f", v->data.d);break;
				case type_string: printf("%s", v->data.s);break;
				default:
					printf("Unknown type!\n");
					exit(1);
			}

			val_put(v);
		}
		n = n->next;
	} while(n);

	printf("\n");

	return 0;
}

static int interpret_condition(struct ast_entry *e) {
	struct value *a, *b;

	a = eval(e->child->child);
	b = eval(e->child->next->child);

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

	val_put(a);
	val_put(b);

	return 0;

out_true:
	val_put(a);
	val_put(b);

	return 1;
}

struct value *interpret_function(struct ast_entry *e) {
	struct ast_entry *n = e->child;
	int nr = e->children - 2;
	struct value *v, *l;

	n = n->next; // Skip FN name

	while(nr) {
		l = lookup_var(n->val->data.s);
		v = val_pop();
		l->data.i = v->data.i;
		val_put(v);
		n = n->next;
		nr--;
	}

	interpret_block(n);

	v = val_pop();  /* Callers responsibility to val_put() */

	return v;

}

static int interpret_statement(struct ast_entry *e) {
	struct ast_entry *n = e->child;
	int r;

//	printf("AST entry %d (%s)\n", e->id, sym_from_id(e->id)?sym_from_id(e->id)->name:"");

	switch (e->id) {
		case tokn_repeat:
			do {
				r = interpret_block(n);
				if(r)
					return r;
			} while (!interpret_condition(n->next));
			break;
		case tokn_while:
			while (interpret_condition(n)) {
				r = interpret_block(n->next);
				if(r)
					return r;
			}
			break;
		case tokn_if:
			if(interpret_condition(n))
				interpret_block(n->next);
			else
				interpret_block(n->next->next);
			break;
		case tokn_print:
			interpret_print(n);
			break;
		case tokn_assign:
			interpret_assign(n);
			break;
		case tokn_end:
			exit(0);
		case ast_expression: //FIXME: this is a bit iffy.
				val_push(eval(n)); //FIXME: null return?
			break;
		default:
			printf("Unexpected AST entry %d (%s)\n", e->id, sym_from_id(e->id)?sym_from_id(e->id)->name:"");
			exit(1);
	}

	return 0;
}

static int interpret_block(struct ast_entry *e) {
	struct ast_entry *n = e->child;
	int r;

	do {
		r = interpret_statement(n);

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
			interpret_block(n);
		else {
			printf("invalid AST entry\n");
			exit(1);
		}

		n=n->next;
	} while (n);
}
