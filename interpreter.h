void interpret(struct ast_entry *p);
struct value *interpret_function(struct ast_entry *e);
struct value *lookup_var(char *name);
void val_push(struct value *v);
struct value *val_alloc(void);

