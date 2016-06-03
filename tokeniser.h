#ifndef TOKENISER_H_INCLUDED

#include <assert.h>

enum tokid {
	tokn_if, tokn_then, tokn_else, tokn_endif, tokn_case,
	tokn_of, tokn_when, tokn_endcase, tokn_on, tokn_error,
	tokn_proc, tokn_fn, tokn_endproc, tokn_return, tokn_end,
	tokn_fork, tokn_for, tokn_each, tokn_in, tokn_to,
	tokn_step, tokn_next, tokn_repeat, tokn_until, tokn_while,
	tokn_endwhile, tokn_break, tokn_continue, tokn_goto, tokn_static,
	tokn_global, tokn_const, tokn_not, tokn_or, tokn_and,
	tokn_xor, tokn_true, tokn_false, tokn_def, tokn_let,
	tokn_be, tokn_as, tokn_signed, tokn_unsigned, tokn_value,
	tokn_bit, tokn_alloc, tokn_dim, tokn_list, tokn_add,
	tokn_before, tokn_after, tokn_remove, tokn_library, tokn_eol,
	tokn_comma, tokn_print, tokn_input, tokn_semicolon,
	tokn_lt, tokn_gt, tokn_le, tokn_ge, tokn_ne,
	tokn_eq, tokn_abs, tokn_mod, tokn_div, tokn_asterisk,
	tokn_slash, tokn_plus, tokn_minus, tokn_lshift, tokn_rshift,
	tokn_colon, tokn_at, tokn_oparen, tokn_cparen, tokn_obrace,
	tokn_cbrace, tokn_percent, tokn_dollar,	tokn_label, tokn_otherwise,
	tokn_comment,
	/* These tokens will never be emitted by the tokeniser and are
	 * for the use of the parser only.
	 */
	tokn_uminus, tokn_uplus,
	/* Special tokens used in the AST */
	ast_program, ast_block, ast_proc, ast_fn, ast_in_param, ast_expression,
};

union storage {
	void *v;
	char *s;
	float d;
	int i;
};

enum typeid {
	type_int, type_float, type_string, type_unspec,
};

#define VAL_READONLY (1<<0)
#define VAL_ALLOC    (1<<1)

struct value {
	enum typeid type;
	int flags;
	union storage data;
	struct label *next;
	char *name;
};

struct token {
        struct symbol *sym;
        struct token *next;
	struct value *val;
	int ref;
};

struct symbol {
        enum tokid id;
        char *name;
        struct token *(*tok_func)(struct symbol *s, char **ps);
        void (*print)(struct token *t);
};

#define tokid(a) (a)->sym->id

int tokeniser_init (void);
struct token *get_next_token(int fd);
void tok_print_one(struct token *t);
void tok_print_line(struct token *t);
struct symbol *sym_from_id(enum tokid id);

static inline struct token *tok_get(struct token *t) {
	t->ref++;

	return t;
}

static inline void tok_put(struct token *t) {
	t->ref--;

	assert(t->ref >= 0);

	if(t->ref == 0) {
		free(t->val);
		free(t);
	}
}

#define TOKENISER_H_INCLUDED
#endif

