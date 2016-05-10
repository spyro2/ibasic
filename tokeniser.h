
enum tokid {
	tokn_if = 1, tokn_then, tokn_else, tokn_endif, tokn_case, tokn_of, tokn_when, tokn_endcase, tokn_on, tokn_error, tokn_proc, tokn_fn, tokn_endproc, tokn_return, tokn_end, tokn_fork, tokn_for, tokn_each, tokn_in, tokn_to, tokn_step, tokn_next, tokn_repeat, tokn_until, tokn_while, tokn_endwhile, tokn_break, tokn_continue, tokn_goto, tokn_static, tokn_global, tokn_const, tokn_not, tokn_or, tokn_and, tokn_xor, tokn_true, tokn_false, tokn_def, tokn_let, tokn_be, tokn_as, tokn_signed, tokn_unsigned, tokn_int, tokn_bit, tokn_alloc, tokn_dim, tokn_list, tokn_add, tokn_before, tokn_after, tokn_remove, tokn_library, tokn_newline, tokn_comma, tokn_string, tokn_print, tokn_input, tokn_semicolon, tokn_lt, tokn_gt, tokn_le, tokn_ge, tokn_ne, tokn_eq, tokn_abs, tokn_mod, tokn_div, tokn_asterisk, tokn_slash, tokn_plus, tokn_minus, tokn_lshift, tokn_rshift, tokn_colon, tokn_at, tokn_oparen, tokn_cparen, tokn_obrace, tokn_cbrace, tokn_percent, tokn_dollar,
	tokn_label,
};

struct line_entry {
        struct token *tok;
        struct line_entry *next;
        void *data;
};

struct token {
        enum tokid id;
        char *name;
        struct line_entry *(*tok_func)(struct token *t, char **s);
        void (*print)(struct line_entry *le);
};

int tokeniser_init (void);
struct line_entry *get_next_le(int fd, struct line_entry *jump);
void tok_print_one(struct line_entry *le);
void tok_print_line(struct line_entry *le);

