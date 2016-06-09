void interpret(struct ast_entry *p);
int interpret_block(struct ast_entry *e, struct value *r);

struct value *lookup_var(char *name);
//void val_push(struct value *v);
struct value *val_alloc(char *name);
struct value *val_pop(void);

