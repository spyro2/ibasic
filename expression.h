#include "stack.h"
#include "ast.h"

struct value *val_alloc(void);
struct value *eval(struct ast_entry *o);
struct ast_entry *basic_eval(struct stack *o);
