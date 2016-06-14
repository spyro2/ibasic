#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokeniser.h"
#include "ast.h"

struct ast_table {
	char *name;
	struct ast_entry *a;
};

static struct ast_table *ast[10];

void ast_index(struct ast_entry *a, char *name) {
	int i;

	for(i = 0 ; i < 10 ; i++) {
		if(!ast[i])
			break;

		if(!strcmp(ast[i]->name, name) && ast[i]->a->id == a->id) {
			printf("ast_entry duplicate\n");
			exit(1);
		}
	}

	if(i < 10) {
		ast[i] = malloc(sizeof(*ast[i]));
		ast[i]->name = malloc(strlen(name)+1);
		strcpy(ast[i]->name, name);
		ast[i]->a = a;
	}
	else {
		printf("ast_index full\n");
		exit(1);
	}
}

struct ast_entry *ast_lookup(struct ast_entry *a){
	char *name = a->child->val->data.s;
	int i, id;

	switch (a->id) {
		case tokn_proc: id = ast_proc; break;
		case tokn_fn:   id = ast_fn; break;
		default:
			printf("Invalid lookup\n");
			exit(1);
	}

	for(i = 0 ; i < 10 ; i++) {
		if(ast[i] && !strcmp(ast[i]->name, name) && ast[i]->a->id == id)
			return ast[i]->a;
	}

	printf("Could not find %s%s)\n", a->id == tokn_fn?"function (FN":"procedure (PROC", name);

	return NULL;
}


static struct ast_entry *ast_this;

struct ast_entry *ast_get_context(void) {
	return ast_this;
}

struct ast_entry *ast_new_context(int id) {
	struct ast_entry *a = calloc(1, sizeof(*a));

	a->id = id;
	ast_this = a;

	return a;
}

void ast_set_context(struct ast_entry *a) {
	ast_this = a;
}

struct ast_entry *ast_alloc() {
	struct ast_entry *a = calloc(1, sizeof(*a));

	a->parent = ast_this;
	ast_this->children++;

	if(!ast_this->child)
		ast_this->child = a;
	else
		ast_this->last_child->next = a;

	ast_this->last_child = a;

	return a;
}

struct ast_entry *ast_emit(struct token *t) {
	struct ast_entry *a = ast_alloc();
	//FIXME: alloc failure

	a->id = t->id;
	if(t->val) {
		val_get(t->val);
		a->val = t->val;
	}
	ast_this = a;

	return a;
}

struct ast_entry *ast_emit_leaf(struct token *t) {
	struct ast_entry *a = ast_alloc();
	//FIXME: alloc failure

	a->id = t->id;
	if(t->val) {
		val_get(t->val);
		a->val = t->val;
	}

	return a;
}

void ast_close(void) {
	ast_this = ast_this->parent;
}

void ast_emit_block(void) {
	struct ast_entry *a = ast_alloc();
	//FIXME: alloc failure

	a->id = ast_block;
	ast_this = a;
}

void ast_append(struct ast_entry *a) {

	if(!a) {
		printf("Skipping NULL tree \n");
		return;
	}

	a->parent = ast_this;
	ast_this->children++;

	if(!ast_this->child)
		ast_this->child = a;
	else
		ast_this->last_child->next = a;

	ast_this->last_child = a;
}

static inline void a_ind(int l) {
	for(; l > 0 ; l--)
		printf("\t");
}

static void ast_print_value(struct value *v) {
	switch (v->type) {
		case type_int: printf("<int> %d\n", v->data.i); break;
		case type_float: printf("<float> %f\n", v->data.d); break;
		case type_string: printf("<string> \"%s\"\n", v->data.s); break;
		default: printf("<unknown value>\n");break;
	}
}

static void ast_print_one(struct ast_entry *a, int l) {
	struct ast_entry *c = a->child;
	struct value *v = a->val;

	if(!c) {
		a_ind(l);
		if(a->id == tokn_value)
			ast_print_value(v);
		else if(a->id == tokn_label)
			printf("<label> %s\n", v->data.s);
		else
			printf("%s\n", sym_from_id(a->id));
	}
	else {
		a_ind(l);

		printf("%s [%d] (\n", sym_from_id(a->id), a->children);

		while(c) {
			struct ast_entry *n = c->next;
			ast_print_one(c, l+1);
			c = n;
		}

		a_ind(l);
		printf(")\n");
	}
}

void ast_print_tree(struct ast_entry *a) {
	ast_print_one(a, 0);
}

static void do_ast_free_tree(struct ast_entry *a) {
	struct ast_entry *c = a->child;

	while(c) {
		struct ast_entry *n = c->next;
		do_ast_free_tree(c);
		c = n;
	}

	if(a->val)
		val_put(a->val);
	free(a);
}

void ast_free_tree(struct ast_entry *a) {
	do_ast_free_tree(a);
}

void ast_exit(void) {
	int i;

	for(i = 0 ; i < 10 ; i++) {
		if(!ast[i])
			break;
		free(ast[i]->name);
		free(ast[i]);
	}
}
