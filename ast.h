#ifndef AST_H_INCLUDED

#include "tokeniser.h"

struct ast_entry {
	struct ast_entry *next;
	struct ast_entry *child;
	struct ast_entry *last_child;
	struct ast_entry *parent;
	int children;
	enum tokid id;
	struct imm_value *val;
};

void ast_index(struct ast_entry *a, char *b);
struct ast_entry *ast_lookup(struct ast_entry *a);

struct ast_entry *ast_get_context(void);
struct ast_entry *ast_new_context(int id);
void ast_set_context(struct ast_entry *a);

struct ast_entry *ast_emit(struct token *t);
struct ast_entry *ast_emit_leaf(struct token *t);
struct ast_entry *ast_emit_leaf_after(struct ast_entry *n, struct token *t);
void ast_close(void);
void ast_emit_block(void);
void ast_append(struct ast_entry *a);
void ast_append_after(struct ast_entry *n, struct ast_entry *a);

void ast_print_tree(struct ast_entry *a);
void ast_free_tree(struct ast_entry *a);

void ast_exit(void);

#define AST_H_INCLUDED
#endif
