#include "stack.h"

struct value *val_alloc(void);
struct value *eval(struct stack *o);
struct ast_entry *basic_eval(struct stack *o);
