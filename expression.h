#include "stack.h"

struct value *eval(struct stack *o);
void print_expression(struct stack *output);
struct ast_entry *basic_eval(struct stack *o);
