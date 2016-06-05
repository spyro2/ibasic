#include <stdio.h>

#include "ast.h"

void interpret(struct ast_entry *p) {
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

}
