#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "tokeniser.h"

static struct line_entry *le_alloc(int len) {
	struct line_entry *le = malloc(sizeof(*le)+len);

	if(len)
		le->data.v = le+1;

	return le;
}

/* TODO: Think about ways to return errors, eg. when adding escape parsing,
 * how to handle bad escape sequences
 */
static struct line_entry *tokfn_string(struct symbol *t, char **ps) {
	char *s = *ps;
	char *dest;
	int len;
	struct line_entry *le = NULL;

	/* FIXME: we really need to handle end of line better here */
	while(*s && *s != '"' && *s != '\n' && *s != '\r')
		s++;

	// if(!*s)  FIXME: check for end of line / non-string chars

	len = s-*ps;

	le = le_alloc(len+1); // FIXME: Check failure

	dest = le->data.s;
	memcpy(dest, *ps, len);
	dest[len] = 0;

	le->sym = t;

	*ps = ++s;

	return le;
}

static void print_string(struct line_entry *le) {
	if(le->data.s)
		printf("\"%s\"", le->data.s);
}

/* FIXME: Terrible hack to allow at least single line comments */
static struct line_entry *tokfn_comment(struct symbol *t, char **ps) {
	char *s = *ps;
	char *dest;
	int len;
	struct line_entry *le = NULL;

	/* FIXME: we really need to handle end of line better here */
	while(*s && !(*s == '*' && *(s+1) == '/'))
		s++;

	// if(!*s)  FIXME: check for end of line / non-string chars

	len = s-*ps;

	s++;

	le = le_alloc(len+1); // FIXME: Check failure

	dest = le->data.s;
	memcpy(dest, *ps, len);
	dest[len] = 0;

	le->sym = t;

	*ps = ++s;

	return le;
}

static void print_eol(struct line_entry *le) {
	printf(" <EOL>");
}

static struct line_entry *default_tokfn(struct symbol *t, char **ps) {
	struct line_entry *le = le_alloc(0); //FIXME: alloc failure
	le->sym = t;

	return le;
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

static void print_label(struct line_entry *le) {
	if(le->data.s)
		printf("%s ", le->data.s);
}

static void print_int(struct line_entry *le) {
	printf("%d ", le->data.i);
}

static void print_float(struct line_entry *le) {
	printf("%f ", le->data.d);
}

static struct symbol sym_label = {tokn_label, "<label>", NULL, print_label};
static struct symbol sym_int   = {tokn_int,   "<int>", NULL, print_int};
static struct symbol sym_float = {tokn_float, "<float>", NULL, print_float};

static struct line_entry *extract_label(char **ps) {
	char *s = *ps;
	struct line_entry *le = NULL;
	int len;
	char *dest;

	while(*s && IS_LABEL(*s)) {
		s++;
	}

	/* We cannot handle zero length labels. */
	if ( s == *ps )
		goto out;

	len = s-*ps;

	/* Numbers */
	if ( IS_DIGIT(**ps) ) {
		char *sc = *ps;
		char n = len>1?sc[1]:0;

		le = le_alloc(0);

		if(*sc == '0') {
			if (n == 'x') { /* hex */
				if(len < 3)
					goto out_free;
				le->sym = &sym_int;
				le->data.i = strtoul(*ps, ps, 0);
				goto out;
			}
			else if(IS_DIGIT(n)) { /* octal */
				le->sym = &sym_int;
				le->data.i = strtoul(*ps, ps, 0);
				goto out;
			}
		}
		else if(strchr(*ps, '.')) { /* decimal */
			le->sym = &sym_float;
			le->data.d = strtod(*ps, ps);
			goto out;
		}

		/* ordinary integer */
		le->sym = &sym_int;
		le->data.i = strtoul(*ps, ps, 10);

		goto out;
	}

	/* Labels may not contain . */
	if(strchr(*ps, '.'))
		goto out;

	le = le_alloc(len+1); // FIXME: Check failure

	dest = le->data.s;
	memcpy(dest, *ps, len);
	dest[len] = 0;

	le->sym = &sym_label;

	*ps = s;

	return le;

out_free:
	free(le);
	le = NULL;
out:
	return le;
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

static struct line_entry *tokenise(struct sym_tree_entry *sym_tree, char *string) {
	struct sym_tree_entry *ste;
	struct symbol *s;
	char *r = string, *b;
	struct line_entry *l = NULL, *le, **pl = &l;

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
				le = s->tok_func(s, &r);
			else
				le = default_tokfn(s, &r);
		}
		else {     /* Non-symbol thing found */
			le = extract_label(&r);
			if(!le) {
				printf("Bad Label\n");
				exit(1);
			}
		}

		/* Handle errors here. FIXME: dont silently skip badness */

		if(le) {
			*pl = le;
			pl = &le->next;
		}
	}

	return l;
}

void tok_print_one(struct line_entry *le) {

	if(le->sym->print)
		le->sym->print(le);
	else
		printf("%s ", le->sym->name);
}

void tok_print_line(struct line_entry *le) {

	while(le) {
		tok_print_one(le);
		le = le->next;
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
		end = read(fd, buf, 32*1024);
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


static struct line_entry *attempt_to_get_le(struct sym_tree_entry *sym_tree, int fd) {
	char *buf;
	struct line_entry *le;

	buf = get_one_line(fd); // FIXME: for now, guarantee newline  terminated or die.

	if(!buf) {
		printf("Line way too long\n");
		exit(1);
	}

	le = tokenise(sym_tree, buf);

	return le;
}

struct line_entry *get_next_le(int fd, struct line_entry *jump) {
	static struct line_entry *next_le = NULL;
	struct line_entry *le;

	if(next_le)
		le = next_le;
	else
		le = attempt_to_get_le(sym_tree, fd);

	if(le)
		next_le = le->next;

	return le;
}

/* This is not quite a 1:1 mapping- may need FIXME in future */

struct symbol *sym_from_id(enum tokid id) {
	struct symbol *t = symbol_list;

        while(t->name) {
		if(t->id == id)
			return t;
		t++;
	}

	return NULL;
}

/*
 * TODO: Add code to make a "flattened" tree, which can be parsed more quickly
 * As an optimisation, store "tree row length" and keep entries in alpha order,
 * allowing faster matching of symbols (binary search)
 */

int tokeniser_init (void) {
	struct symbol *t;

	t = symbol_list;

	while(t->name) {
		if(sym_add(&sym_tree, t, t->name)) {
			printf("Failed whilst adding symbol: \"%s\"", t->name);
			exit(1);
		}
		t++;
	}

	return 0;
}
