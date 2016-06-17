#ifndef TOKENISER_H_INCLUDED

#include <stdlib.h>
#include <assert.h>

#include "types.h"

enum tokid {
/*0*/	tokn_if, tokn_then, tokn_else, tokn_endif, tokn_case,
	tokn_of, tokn_when, tokn_endcase, tokn_on, tokn_error,
/*10*/	tokn_proc, tokn_fn, tokn_endproc, tokn_return, tokn_end,
	tokn_fork, tokn_for, tokn_each, tokn_in, tokn_to,
/*20*/	tokn_step, tokn_next, tokn_repeat, tokn_until, tokn_while,
	tokn_endwhile, tokn_break, tokn_continue, tokn_goto, tokn_static,
/*30*/	tokn_global, tokn_const, tokn_not, tokn_or, tokn_and,
	tokn_xor, tokn_true, tokn_false, tokn_def, tokn_let,
/*40*/	tokn_be, tokn_as, tokn_signed, tokn_unsigned, tokn_value,
	tokn_bit, tokn_alloc, tokn_dim, tokn_list, tokn_add,
/*50*/	tokn_before, tokn_after, tokn_remove, tokn_library, tokn_eol,
	tokn_comma, tokn_print, tokn_input, tokn_semicolon, tokn_lt,
/*60*/	tokn_gt, tokn_le, tokn_ge, tokn_ne, tokn_eq,
	tokn_abs, tokn_mod, tokn_div, tokn_asterisk, tokn_slash,
/*70*/	tokn_plus, tokn_minus, tokn_lshift, tokn_rshift, tokn_colon,
	tokn_at, tokn_oparen, tokn_cparen, tokn_obrace, tokn_cbrace,
/*80*/	tokn_percent, tokn_dollar, tokn_label, tokn_otherwise, tokn_comment,
	tokn_osquare, tokn_csquare, tokn_local, tokn_eof,
	/* These tokens will never be emitted by the tokeniser and are
	 * for the use of the parser only.
	 */
	tokn_uminus, tokn_uplus, tokn_assign, tokn_array,
	/* Special tokens used in the AST */
	ast_program, ast_block, ast_proc, ast_fn, ast_expression,
};

struct token {
	enum tokid id;
        struct token *next;
	struct imm_value *val;
	int ref;
};

struct symbol {
        enum tokid id;
        char *name;
        struct token *(*tok_func)(struct symbol *s, char **ps);
};

int tokeniser_init (void);
void tokeniser_exit(void);
struct token *get_next_token(int fd);

char *sym_from_id(enum tokid id);

static inline struct imm_value *val_get(struct imm_value *v) {
	v->ref++;

	return v;
}

static inline void val_put(struct imm_value *v) {
	v->ref--;

	assert(v->ref >= 0);

	if(v->ref == 0)
		free(v);
}

static inline struct token *tok_get(struct token *t) {
	t->ref++;

	if(t->val)
		val_get(t->val);

	return t;
}

static inline void tok_put(struct token *t) {
	t->ref--;

	assert(t->ref >= 0);

	val_put(t->val);

	if(t->ref == 0)
		free(t);
}

#define TOKENISER_H_INCLUDED
#endif

