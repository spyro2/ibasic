void interpret(struct ast_entry *p);
int interpret_block(struct ast_entry *e, struct value *r);

struct value *lookup_var(char *name);
struct value *val_alloc(char *name);
struct value *val_alloc_frame(void);
void val_set_frame(struct value *v);
void unwind_frame(void);
struct value *val_pop(void);

