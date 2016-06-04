#include <stdio.h>
#include <stdlib.h>

#include "tokeniser.h"

struct ast_entry {
	struct ast_entry *next;
	struct ast_entry *child;
	struct ast_entry *last_child;
	struct ast_entry *parent;
	int children;
	enum tokid id;
	struct value *val;
};

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
	struct symbol *s = t->sym;
	struct ast_entry *a = ast_alloc();
	//FIXME: alloc failure

	a->id = s->id;
	a->val = t->val;
	ast_this = a;

	return a;
}

struct ast_entry *ast_emit_leaf(struct token *t) {
	struct symbol *s = t->sym;
	struct ast_entry *a = ast_alloc();
	//FIXME: alloc failure

	a->id = s->id;
	a->val = t->val;

	return a;
}

void ast_close(void) {
	ast_this = ast_this->parent;
}

void ast_index(struct ast_entry *a, char *b) {
	printf("index(%s)\n", b);
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

static void ast_print_value(struct ast_entry *a) {
	struct value *v = a->val;

	switch (v->type) {
		case type_int: printf("<int> %d\n", v->data.i); break;
		case type_float: printf("<float> %f\n", v->data.d); break;
		case type_string: printf("<string> \"%s\"\n", v->data.s); break;
		default: printf("<unknown value>\n");break;
	}
}

static char *ast_name(enum tokid id) {
	switch(id) {
		case ast_program: return "ast_program";
		case ast_block: return "ast_block";
		case ast_proc: return "ast_proc";
		case ast_fn: return "ast_fn";
		case ast_expression: return "ast_expression";
		default: return "ast_unknown";
	}
}

static void ast_print_one(struct ast_entry *a, int l) {
	struct ast_entry *c = a->child;
	struct symbol *s;

	s = sym_from_id(a->id);

	if(!c) {
		a_ind(l);
		if(a->id == tokn_value)
			ast_print_value(a);
		else if(a->id == tokn_label)
			printf("<label> %s\n", a->val->data.s);
		else if(a->id == tokn_oparen)
			printf("empty_group\n");
		else if(s && s->name)
			printf("%s\n", s->name);
		else
			printf("%s\n", ast_name(a->id));
	}
	else {
		a_ind(l);
		if(a->id == tokn_assign)
			printf("assign[%d] (\n", a->children);
		else if(a->id == tokn_oparen)
			printf("group[%d] (\n", a->children);
		else if(s && s->name)
			printf("%s[%d] (\n", s->name, a->children);
		else
			printf("%s[%d] (\n", ast_name(a->id), a->children);

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
	exit(0);
}
