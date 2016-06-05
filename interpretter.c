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
	}

	val_get(var[i]);

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

static int interpret_statement(struct ast_entry *e) {
	struct ast_entry *n = e->child;
	int r;

//	printf("AST entry %d (%s)\n", e->id, sym_from_id(e->id)?sym_from_id(e->id)->name:"");

	switch (e->id) {
		case tokn_assign:
			interpret_assign(n);
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
