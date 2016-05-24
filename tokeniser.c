#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "tokeniser.h"

static struct token *tok_alloc(int len) {
	struct token *t = malloc(sizeof(*t)+len);

	if(len)
		t->data.v = t+1;

	return t;
}

/* TODO: Think about ways to return errors, eg. when adding escape parsing,
 * how to handle bad escape sequences
 */
static struct token *tokfn_string(struct symbol *s, char **ps) {
	char *r = *ps;
	int len;
	struct token *t = NULL;

	/* FIXME: we really need to handle end of line better here */
	while(*r && *r != '"' && *r != '\n' && *r != '\r')
		r++;

	// if(!*s)  FIXME: check for end of line / non-string chars

	len = r-*ps;

	t = tok_alloc(len+1); // FIXME: Check failure

	memcpy(t->data.s, *ps, len);
	t->data.s[len] = 0;

	t->sym = s;

	*ps = ++r;

	return t;
}

static void print_string(struct token *t) {
	if(t->data.s)
		printf("\"%s\"", t->data.s);
}

/* FIXME: Terrible hack to allow at least single line comments */
static struct token *tokfn_comment(struct symbol *s, char **ps) {
	char *r = *ps;
	int len;
	struct token *t = NULL;

	/* FIXME: we really need to handle end of line better here */
	while(*r && !(*r == '*' && *(r+1) == '/'))
		r++;

	// if(!*s)  FIXME: check for end of line / non-string chars

	len = r-*ps;

	r++;

	t = tok_alloc(len+1); // FIXME: Check failure

	memcpy(t->data.s, *ps, len);
	t->data.s[len] = 0;

	t->sym = s;

	*ps = ++r;

	return t;
}

static void print_eol(struct token *t) {
	printf(" <EOL>");
}

static struct token *default_tokfn(struct symbol *s, char **ps) {
	struct token *t = tok_alloc(0); //FIXME: alloc failure

	t->sym = s;

	return t;
}

static struct symbol symbol_list[] = {
	/* Flow control and error handling */
	{tokn_if, "IF",},
	{tokn_then, "THEN",},
	{tokn_else, "ELSE",},
	{tokn_endif, "ENDIF",},
	{tokn_case, "CASE",},
	{tokn_of, "OF",},
	{tokn_when, "WHEN",},
	{tokn_otherwise, "OTHERWISE",},
	{tokn_endcase, "ENDCASE",},
/*	{tokn_on, "ON",},
	{tokn_error, "ERROR",}, */
	{tokn_proc, "PROC",},
	{tokn_fn, "FN",},
	{tokn_endproc, "ENDPROC",},
	{tokn_return, "RETURN",}, /* Call by reference */
	{tokn_end, "END",},

	/* Threading! */
/*	{tokn_fork, "FORK",}, */

	/* Loops */
#if 0
	{tokn_for, "FOR",},
	{tokn_each, "EACH",}, /* For lists */
	{tokn_in, "IN",},
	{tokn_to, "TO",},
	{tokn_step, "STEP",},
	{tokn_next, "NEXT",},
#endif
	{tokn_repeat, "REPEAT",},
	{tokn_until, "UNTIL",},
	{tokn_while, "WHILE",},
	{tokn_endwhile, "ENDWHILE",},

	/* Evil awkkward cases */
	{tokn_break, "BREAK",},
	{tokn_continue, "CONTINUE",},
	{tokn_goto, "GOTO",},

	/* Scoping */
	{tokn_static, "STATIC",},
	{tokn_global, "GLOBAL",},
	{tokn_const, "CONST",},

	/* Logic operators */
	{tokn_not, "NOT",},
	{tokn_or, "OR",},
	{tokn_and, "AND",},
	{tokn_xor, "XOR",},
	{tokn_true, "TRUE",},  /* Handle these as special labels in future */
	{tokn_false, "FALSE",},

	/* Define functions / procedures / structures / types */
	{tokn_def, "DEF",},

#if 0
	/* Allow for more sophisticated types without sigil-hell */
	{tokn_let, "LET",},
	{tokn_be, "BE",},
	{tokn_as, "AS",},
	{tokn_signed, "SIGNED",},
	{tokn_unsigned, "UNSIGNED",},
	{tokn_int, "INT",},
	{tokn_bit, "BIT",},
#endif

/*	{ "MACRO",}, */

	/* Memory allocation */
/*	{tokn_alloc, "ALLOC",}, */
#if 0
	{tokn_dim, "DIM",},
	{tokn_list, "LIST",}, /* Unlike older BASICs - for creating lists! */
	{tokn_add, "ADD",},
	{tokn_before, "BEFORE",},
	{tokn_after, "AFTER",},
	{tokn_remove, "REMOVE",},
#endif
	/* SIZEOF */

	/* Allow loading of additional libraries at runtime */
	{tokn_library, "LIBRARY",},

	/* Seperators and EOL */
	{tokn_eol, "\r\n", NULL, print_eol},
	{tokn_eol, "\n", NULL, print_eol},
	{tokn_comma, ",",},

	/* Types */
	{tokn_string, "\"", tokfn_string, print_string},

	/* IO statements */
	{tokn_print, "PRINT",},
	{tokn_input, "INPUT",},

	/* String operators */
	{tokn_semicolon, ";",},
	/*
	LEN
	MID
	LEFT
	RIGHT
	INSTR
	STRING
	*/

	/* Comparison operators */
	{tokn_lt, "<",},
	{tokn_gt, ">",},
	{tokn_le, "<=",},
	{tokn_ge, ">=",},
	{tokn_ne, "<>",},

	/* Comparison & assignment */
	{tokn_eq, "=",}, /* exactly equal - perhaps need a '==' for equivalent? */

	/* Brackets */
	{tokn_oparen, "(",},
	{tokn_cparen, ")",},
	{tokn_obrace, "{",},
	{tokn_cbrace, "}",},

	/* Math operators */
	/* An = char after these makes them assign the result into the first
	 * operand. Maybe use a special evaluator for these. Maybe not.
	 */
	{tokn_abs, "ABS",},
	{tokn_mod, "MOD",},
	{tokn_div, "DIV",},
	{tokn_asterisk, "*",},
	{tokn_slash, "/",},
	{tokn_plus, "+",},
	{tokn_minus, "-",}, /* negation / subtraction */
	{tokn_lshift, "<<",},
	{tokn_rshift, ">>",}, /* Consider a >>> for non-twos complement shift */

	/* BASIC's sigil types are horrible and limiting - perhaps we
	 * should remove/simplify them?
	 */
/*	{ "?",}, prefix: indirect u8       suffix: u8  variable */
	{tokn_percent, "%",}, /* prefix: indirect u16      suffix: u16 variable */
/*	{ "!",}, prefix: indirect u32      suffix: u32 variable */
	{tokn_dollar, "$",}, /* prefix: indirect string   suffix: string variable */
/*	{ "#",}, prefix: indirect double   suffix: double variable */
/*	{ "^",}, prefix: indirection operator suffix: pointer variable */
/*	{ "|",}, */
	{tokn_colon, ":",},
/*	{ "~",}, */
/*	{ "&",}, */
	{tokn_at, "@",}, /* prefix: system variables */

	/* Comments */
	{tokn_comment, "/*", tokfn_comment},

	/* special cases in the lexer - NOT IMPLEMENTED YET */
	/* \ - line continuation */

	/* 0-9 - numbers */
	/* 0n (where n is one or more digits) - octal */
	/* 0xn (where n is one or more hex digits */
	/* 0bn (where n is one or more binary digits */

	/* Anything else non-matching - labels */
	{0, NULL},
};

struct sym_tree_entry {
	char c;
	struct sym_tree_entry *next;
	struct sym_tree_entry *children;
	struct symbol *sym;
};

static struct sym_tree_entry *sym_tree = NULL;

/*
 * We assume the symbol being added is valid - this function is recursive and
 * so we dont know if we are being called initially or are already at some
 * stage in the process of adding a symbol
 */

static int sym_add(struct sym_tree_entry **pste, struct symbol *s, char *c) {
	struct sym_tree_entry *ste;

	if(!pste)
		return -EINVAL;

	ste = *pste;

	/* Are we creating a new entry? */
	if(!ste) {
		struct sym_tree_entry *ste_new;
		int ret = 0;

		ste_new = calloc(1, sizeof(*ste_new));

		if(!ste_new)
			return -ENOMEM;

		ste_new->c = *c++;

		/*
		 * If this is the last character, add a pointer to the symbol
		 * descriptor.
		 * Otherwise, keep recursing...
		 */
		if(!*c)
			ste_new->sym = s;
		else
			ret = sym_add(&ste_new->children, s, c);

		/*
		 * If adding a child entry failed, clean up.
		 * Otherwise, link the entry to the tree.
		 */
		if(ret == -ENOMEM)
			free(ste_new);
		else
			*pste = ste_new;

		return ret;
	}

	while(ste) {
		if(ste->c == *c) { /* Entry exists, descend. */
			c++;
			if(!*c)
				if(!ste->sym) {
					ste->sym = s;
					return 0;
				}
				else /* Entry is a duplicate */
					return 1;
			else    /* Add new entry */
				return sym_add(&ste->children, s, c);
		}
		/* No existing entry, recursively add */
		else if(!ste->next)
				return sym_add(&ste->next, s, c);

		ste = ste->next;
	}

	return 1; /* Shut compiler up */
}

#define IS_WS(c) ((c)==' ' || (c)=='\t')
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define IS_LABEL(c) (IS_DIGIT(c) || IS_ALPHA(c) || ((c) == '_') || ((c) == '.'))

static void print_label(struct token *t) {
	if(t->data.s)
		printf("%s ", t->data.s);
}

static void print_int(struct token *t) {
	printf("%d ", t->data.i);
}

static void print_float(struct token *t) {
	printf("%f ", t->data.d);
}

static struct symbol sym_label = {tokn_label, "<label>", NULL, print_label};
static struct symbol sym_int   = {tokn_int,   "<int>", NULL, print_int};
static struct symbol sym_float = {tokn_float, "<float>", NULL, print_float};

static struct token *extract_label(char **ps) {
	char *r = *ps;
	struct token *t = NULL;
	int len;

	while(*r && IS_LABEL(*r)) {
		r++;
	}

	/* We cannot handle zero length labels. */
	if ( r == *ps )
		goto out;

	len = r-*ps;

	/* Numbers */
	if ( IS_DIGIT(**ps) ) {
		char *sc = *ps;
		char n = len>1?sc[1]:0;

		t = tok_alloc(0);

		if(*sc == '0') {
			if (n == 'x') { /* hex */
				if(len < 3)
					goto out_free;
				t->sym = &sym_int;
				t->data.i = strtoul(*ps, ps, 0);
				goto out;
			}
			else if(IS_DIGIT(n)) { /* octal */
				t->sym = &sym_int;
				t->data.i = strtoul(*ps, ps, 0);
				goto out;
			}
		}
		else if(strchr(*ps, '.')) { /* decimal */
			t->sym = &sym_float;
			t->data.d = strtod(*ps, ps);
			goto out;
		}

		/* ordinary integer */
		t->sym = &sym_int;
		t->data.i = strtoul(*ps, ps, 10);

		goto out;
	}

	/* Labels may not contain . */
	if(strchr(*ps, '.'))
		goto out;

	t = tok_alloc(len+1); // FIXME: Check failure

	memcpy(t->data.s, *ps, len);
	t->data.s[len] = 0;

	t->sym = &sym_label;

	*ps = r;

	return t;

out_free:
	free(t);
	t = NULL;
out:
	return t;
}

/*
 * Currently, this lexer does store "back references" so that it can
 * correctly parse a label which follows a partially matched symbol, eg.
 * "ENDPROD" instead of ENDPROC (END is a symbol, followed by PROD).
 * This could be avoided by allowing the lexer to copy each character
 * following a symbol into a buffer, until it realises what the next thing is.
 * but this seems un-necessary (as long as it parses whole lines at a time).
 */

/*
 * Perhaps we can skip whitespace using a symbol that is always discarded?
 */

static struct token *tokenise(struct sym_tree_entry *sym_tree, char *string) {
	struct sym_tree_entry *ste;
	struct symbol *s;
	char *r = string, *b;
	struct token *l = NULL, *t, **pl = &l;

	while(*r) {

		/* Skip whitespace */
		while(IS_WS(*r))
			r++;

		/* Give up at the end of the string */
		if(!*r)
			break;

		ste = sym_tree;
		s = NULL;
		b = r; /* Set backtrack point */

		while(ste) {
			if(ste->c == *r) {
				r++;

				/* Potentially found symbol */
				if(ste->sym) {
					s = ste->sym;
					b = r; /* Set backtrack point */
				}

				/* Found a symbol with certainty */
				if(!ste->children || !*r || IS_WS(*r))
					break;

				ste = ste->children;
			}
			else 
				ste = ste->next;
		}

		/*
		 * If we reach this point without t pointing to a symbol
		 * we found a label, string, etc.
		 *
		 * If we have no valid ste (ie. no match with certainty), then
		 * we will need to backtrack in order to remain in sync.
		 *
		 */ 

		if(!ste || !s)
			r = b; /* Backtrack */

		if (s) { /* Found a symbol, create a token */
			if(s->tok_func)
				t = s->tok_func(s, &r);
			else
				t = default_tokfn(s, &r);
		}
		else {     /* Non-symbol thing found */
			t = extract_label(&r);
			if(!t) {
				printf("Bad Label\n");
				exit(1);
			}
		}

		/* Handle errors here. FIXME: dont silently skip badness */

		if(t) {
			*pl = t;
			pl = &t->next;
		}
	}

	return l;
}

void tok_print_one(struct token *t) {

	if(t->sym->print)
		t->sym->print(t);
	else
		printf("%s ", t->sym->name);
}

void tok_print_line(struct token *t) {

	while(t) {
		tok_print_one(t);
		t = t->next;
	}
	printf("\n");

}

static char *get_one_line(int fd) {
	static char *buf;
	static char line[32*1024];
	static int end;
	char *n;

	if(!buf) {
		buf = malloc(32*1024); //FIXME: return code
		end = read(fd, buf, 32*1024); //FIXME: can be merged with later code
	}

	n = buf;
	while (n < buf+end && *n != '\n' ) {
		if(*n == 0) {
			printf("End of program\n");
			exit(0);
		}
		n++;
	}

	if ( n < buf+end ) {
		int r;
		memcpy(line, buf, (n-buf)+1);
		line[n-buf+1] = 0;
		memmove(buf, n+1, 32*1024-(n-buf)+1);
		r = read(fd, n+1, (n-buf)+1);
		if ( r != -1)
			end += r;
		if ( r == 0 )
			buf[end] = 0;
		return line;
	}

	return NULL;
}


static struct token *get_more_tokens(struct sym_tree_entry *sym_tree, int fd) {
	char *buf;
	struct token *t;

	buf = get_one_line(fd); // FIXME: for now, guarantee newline  terminated or die.

	if(!buf) {
		printf("Line way too long\n");
		exit(1);
	}

	t = tokenise(sym_tree, buf);

	return t;
}

struct token *get_next_token(int fd) {
	static struct token *next_tok = NULL;
	struct token *t;

	if(next_tok)
		t = next_tok;
	else
		t = get_more_tokens(sym_tree, fd);

	if(t)
		next_tok = t->next;

	return t;
}

/* This is not quite a 1:1 mapping- may need FIXME in future */

struct symbol *sym_from_id(enum tokid id) {
	struct symbol *s = symbol_list;

        while(s->name) {
		if(s->id == id)
			return s;
		s++;
	}

	return NULL;
}

/*
 * TODO: Add code to make a "flattened" tree, which can be parsed more quickly
 * As an optimisation, store "tree row length" and keep entries in alpha order,
 * allowing faster matching of symbols (binary search)
 */

int tokeniser_init (void) {
	struct symbol *s;

	s = symbol_list;

	while(s->name) {
		if(sym_add(&sym_tree, s, s->name)) {
			printf("Failed whilst adding symbol: \"%s\"", s->name);
			exit(1);
		}
		s++;
	}

	return 0;
}
