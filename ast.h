#include "tokeniser.h"

struct ast_entry *ast_get_context(void);
struct ast_entry *ast_new_context(int id);
void ast_set_context(struct ast_entry *a);

struct ast_entry *ast_emit(struct token *t);
struct ast_entry *ast_emit_leaf(struct token *t);
void ast_close(void);
void ast_index(struct ast_entry *a, char *b);
void ast_emit_block(void);
void ast_append(struct ast_entry *a);

void ast_print_tree(struct ast_entry *a);
void ast_free_tree(struct ast_entry *a);

