#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "tokeniser.h"

static struct line_entry *le_alloc(int len) {
	struct line_entry *le = malloc(sizeof(*le)+len);

	if(len)
		le->data = le+1;

	return le;
}

/* TODO: Think about ways to return errors, eg. when adding escape parsing,
 * how to handle bad escape sequences
 */
static struct line_entry *tokfn_string(struct token *t, char **ps) {
	char *s = *ps;
	char *dest;
	int len;
	struct line_entry *le = NULL;

	while(*s && *s != '"')
		s++;

	// if(!*s)  FIXME: check for end of line / non-string chars

	len = s-*ps;

	le = le_alloc(len+1); // FIXME: Check failure

	dest = (char *)le->data;
	memcpy(dest, *ps, len);
	dest[len] = 0;

	le->tok = t;

	*ps = ++s;

	return le;
}

static void print_string(struct line_entry *le) {
	if(le->data)
		printf("\"%s\"", (char *)le->data);
}

static void print_eol(struct line_entry *le) {
	printf(" <EOL>\n");
}

static struct line_entry *default_tokfn(struct token *t, char **ps) {
	struct line_entry *le = le_alloc(0); //FIXME: alloc failure
	le->tok = t;

	return le;
}

static struct token token_list[] = {
	/* Flow control and error handling */
	{tokn_if, "IF",},
	{tokn_then, "THEN",},
	{tokn_else, "ELSE",},
	{tokn_endif, "ENDIF",},
	{tokn_case, "CASE",},
	{tokn_of, "OF",},
	{tokn_when, "WHEN",},
	{tokn_endcase, "ENDCASE",},
	{tokn_on, "ON",},
	{tokn_error, "ERROR",},
	{tokn_proc, "PROC",},
	{tokn_fn, "FN",},
	{tokn_endproc, "ENDPROC",},
	{tokn_return, "RETURN",}, /* Call by reference */
	{tokn_end, "END",},
	{tokn_fork, "FORK",},  /* Threading! */

	/* Loops */
	{tokn_for, "FOR",},
	{tokn_each, "EACH",}, /* For lists */
	{tokn_in, "IN",},
	{tokn_to, "TO",},
	{tokn_step, "STEP",},
	{tokn_next, "NEXT",},
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

	/* Allow for more sophisticated types without sigil-hell */
	{tokn_let, "LET",},
	{tokn_be, "BE",},
	{tokn_as, "AS",},
	{tokn_signed, "SIGNED",},
	{tokn_unsigned, "UNSIGNED",},
	{tokn_int, "INT",},
	{tokn_bit, "BIT",},

/*	{ "MACRO",}, */

	/* Memory allocation */
	{tokn_alloc, "ALLOC",},
	{tokn_dim, "DIM",},
	{tokn_list, "LIST",}, /* Unlike older BASICs - for creating lists! */
	{tokn_add, "ADD",},
	{tokn_before, "BEFORE",},
	{tokn_after, "AFTER",},
	{tokn_remove, "REMOVE",},
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

	/* special cases in the lexer - NOT IMPLEMENTED YET */
	/* \ - line continuation */
	/* { "/ *",}, - need evaluator for comments */

	/* 0-9 - numbers */
	/* 0n (where n is one or more digits) - octal */
	/* 0xn (where n is one or more hex digits */
	/* 0bn (where n is one or more binary digits */

	/* Anything else non-matching - labels */
	{0, NULL},
};

static void print_label(struct line_entry *le) {
	if(le->data)
		printf("{%s} ", (char *)le->data);
}

static struct token tok_label = {tokn_label, "<label>", NULL, print_label};

struct tok_tree_entry {
	char c;
	struct tok_tree_entry *next;
	struct tok_tree_entry *children;
	struct token *tok;
};

static struct tok_tree_entry *tok_tree = NULL;

/*
 * We assume the token being added is valid - this function is recursive and
 * so we dont know if we are being called initially or are already at some
 * stage in the process of adding a token
 */

static int tok_add(struct tok_tree_entry **ptte, struct token *t, char *c) {
	struct tok_tree_entry *tte;

	if(!ptte)
		return -EINVAL;

	tte = *ptte;

	/* Are we creating a new entry? */
	if(!tte) {
		struct tok_tree_entry *tte_new;
		int ret = 0;

		tte_new = calloc(1, sizeof(*tte_new));

		if(!tte_new)
			return -ENOMEM;

		tte_new->c = *c++;

		/*
		 * If this is the last character, add a pointer to the token
		 * descriptor.
		 * Otherwise, keep recursing...
		 */
		if(!*c)
			tte_new->tok = t;
		else
			ret = tok_add(&tte_new->children, t, c);

		/*
		 * If adding a child entry failed, clean up.
		 * Otherwise, link the entry to the tree.
		 */
		if(ret == -ENOMEM)
			free(tte_new);
		else
			*ptte = tte_new;

		return ret;
	}

	while(tte) {
		if(tte->c == *c) { /* Entry exists, descend. */
			c++;
			if(!*c)
				if(!tte->tok) {
					tte->tok = t;
					return 0;
				}
				else /* Entry is a duplicate */
					return 1;
			else    /* Add new entry */
				return tok_add(&tte->children, t, c);
		}
		/* No existing entry, recursively add */
		else if(!tte->next)
				return tok_add(&tte->next, t, c);

		tte = tte->next;
	}

	return 1; /* Shut compiler up */
}

#define IS_WS(c) ((c)==' ' || (c)=='\t')

#define IS_LABEL(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || ((c) == '_'))

static struct line_entry *extract_label(char **ps) {
	char *s = *ps;
	struct line_entry *le;
	int len;
	char *dest;

	while(*s && IS_LABEL(*s))
		s++;

	/* We cannot handle bad characters, skip them */
	if ( s == *ps ) {
		*ps = ++s;
		return 0;
	}

	len = s-*ps;

	le = le_alloc(len+1); // FIXME: Check failure

	dest = (char *)le->data;
	memcpy(dest, *ps, len);
	dest[len] = 0;

	le->tok = &tok_label;

	*ps = s;

	return le; // This is crap, but non-zero is enough for now.
}

/*
 * Currently, this tokeniser does store "back references" so that it can
 * correctly parse a label which follows a partially matched token, eg.
 * "ENDPROD" instead of ENDPROC (END is a token, followed by PROD).
 * This could be avoided by allowing the tokeniser to copy each character
 * it following a token into a buffer, until it realises what the next thing is.
 * but this seems un-necessary (as long as it parses whole lines at a time).
 */

/*
 * Perhaps we can skip whitespace by making it a token that is always discarded?
 */

static struct line_entry *tokenise(struct tok_tree_entry *tok_tree, char *string) {
	struct tok_tree_entry *tte;
	struct token *t;
	char *s = string, *b;
	struct line_entry *l = NULL, *le, **pl = &l;

	while(*s) {

		/* Skip whitespace */
		while(IS_WS(*s))
			s++;

		/* Give up at the end of the string */
		if(!*s)
			break;

		tte = tok_tree;
		t = NULL;
		b = s; /* Set backtrack point */

		while(tte) {
			if(tte->c == *s) {
				s++;

				/* Potentially found token */
				if(tte->tok) {
					t = tte->tok;
					b = s; /* Set backtrack point */
				}

				/* Found a token with certainty */
				if(!tte->children || !*s || IS_WS(*s))
					break;

				tte = tte->children;
			}
			else 
				tte = tte->next;
		}

		/*
		 * If we reach this point without t pointing to a token
		 * we found a label, string, etc.
		 *
		 * If we have no valid tte (ie. no match with certainty), then
		 * we will need to backtrack in order to remain in sync.
		 *
		 */ 

		if(!tte || !t)
			s = b; /* Backtrack */

		if (t) { /* Found a token */
			if(t->tok_func)
				le = t->tok_func(t, &s);
			else
				le = default_tokfn(t, &s);
		}
		else     /* Non-token thing found */
			le = extract_label(&s);

		/* Handle errors here. FIXME: dont silently skip badness */

		if(le) {
			*pl = le;
			pl = &le->next;
		}
	}

	return l;
}

void tok_print_one(struct line_entry *le) {

	if(le->tok->print)
		le->tok->print(le);
	else
		printf("%s ", le->tok->name);
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


static struct line_entry *attempt_to_get_le(struct tok_tree_entry *tok_tree, int fd) {
	char *buf;
	struct line_entry *le;

	buf = get_one_line(fd); // FIXME: for now, guarantee newline  terminated or die.

	if(!buf) {
		printf("Line way too long\n");
		exit(1);
	}

	le = tokenise(tok_tree, buf);

	/* FIXME: for goto - check if first token is a label? add to label database? or should parser do this? Probably the parser. */

	return le;
}

struct line_entry *get_next_le(int fd, struct line_entry *jump) {
	static struct line_entry *next_le = NULL;
	struct line_entry *le;

	/* FIXME: for goto / procedure calls
	if(jump)
		le = jump_to(jump);
	*/

	if(next_le)
		le = next_le;
	else
		le = attempt_to_get_le(tok_tree, fd);

	if(le)
		next_le = le->next;

	return le;
}

/* This is not quite a 1:1 mapping- may need FIXME in future */

struct token *tok_from_id(enum tokid id) {
	struct token *t = token_list;

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
 * allowing faster matching of tokens (binary search)
 */

int tokeniser_init (void) {
	struct token *t;

	t = token_list;

	while(t->name) {
		if(tok_add(&tok_tree, t, t->name)) {
			printf("Failed whilst adding token: \"%s\"", t->name);
			exit(1);
		}
		t++;
	}

	return 0;
}
