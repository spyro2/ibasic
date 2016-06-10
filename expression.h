#include "stack.h"
#include "ast.h"

struct value *eval(struct ast_entry *o);
void call_proc_or_fn(struct ast_entry *o, struct value *r);
