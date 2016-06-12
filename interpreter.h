void interpret(struct ast_entry *p);
int interpret_block(struct ast_entry *e, struct value *r);

struct value *stack_lookup_var(char *name);

struct value *stack_alloc(char *name);
struct value *stack_pop(void);

struct value *stack_alloc_frame(void);
void stack_set_frame(struct value *v);
void stack_unwind_frame(void);

