#include <stdio.h>

#include "tokeniser.h"

static int ast_ind = 0;

#define ast_indent \
	do { \
		int i; \
		for(i = 0 ; i < ast_ind ; i++) \
			printf("\t"); \
	} while(0)

struct ast_entry *ast_emit(struct token *t) {
	struct symbol *s = t->sym;

	ast_indent;
	switch(s->id) {
		case ast_proc: printf("ast_proc(");break;
		case ast_fn: printf("ast_fn(");break;
		case tokn_oparen: printf("list(");break;
		case tokn_label: printf("<here label>");break;
		case tokn_value: printf("cond (");break;
		default:
			if(s->name)
				printf("%s( ", s->name);
			else
				printf("[%d]( ", s->id);
	}
	printf("\n");
	ast_ind++;

	return NULL;
}

struct ast_entry *ast_emit_leaf(struct token *t) {
	struct symbol *s = t->sym;

	ast_indent;
	switch(s->id) {
		case tokn_value: printf("value, ");break;
		case tokn_label: printf("label, ");break;
		case ast_in_param: printf("in_param, ");break;
		default:
			if(s->name)
				printf("%s, ", s->name);
			else
				printf("[%d], ", s->id);
	}
	printf("\n");

	return NULL;
}

void ast_close(void) {
	ast_ind--;
	ast_indent;
	printf(")\n");
}

void ast_index(struct ast_entry *a, char *b) {
	printf("index(%s)\n", b);
}

void ast_emit_block(void) {
	ast_indent;
	printf("ast_block(\n");
	ast_ind++;
}

void ast_append(struct ast_entry *a) {
	ast_indent;
	printf("<expr>\n");
}

