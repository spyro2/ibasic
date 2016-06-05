#include <stdio.h>

#include "ast.h"

void interpret(struct ast_entry *p) {
static struct value *var[10];


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
