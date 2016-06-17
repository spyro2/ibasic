void interpret(struct ast_entry *p);
int interpret_block(struct ast_entry *e, struct imm_value *r);

struct imm_value *stack_lookup_var(char *name);

struct imm_value *stack_alloc(char *name);
struct imm_value *stack_pop(void);

struct imm_value *stack_alloc_frame(void);
void stack_set_frame(struct imm_value *v);
void stack_unwind_frame(void);

