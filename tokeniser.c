#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct line_entry {
	struct token *tok;
	struct line_entry *next;
	void *data;
};

struct token {
	char *name;
	struct line_entry *(*tok_func)(struct token *t, char **s);
	void (*print)(struct line_entry *le);
};

struct line_entry *le_alloc(int len) {
	struct line_entry *le = malloc(sizeof(*le)+len);

	if(len)
		le->data = le+1;

	return le;
}

/* TODO: Think about ways to return errors, eg. when adding escape parsing,
 * how to handle bad escape sequences
 */
struct line_entry *tokfn_string(struct token *t, char **ps) {
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

void print_string(struct line_entry *le) {
	if(le->data)
		printf("\"%s\"", (char *)le->data);
}

struct line_entry *tokfn_eol(struct token *t, char **ps) {
	char *s = *ps;
	struct line_entry *le = le_alloc(0);
	//FIXME: alloc failure

	/* We accept \r or \r\n as valid EOL so check for a \n */
	if(*s && *s == '\n')
		*ps = ++s;

	le->tok = t;

	return le;
}

struct line_entry *default_tokfn(struct token *t, char **ps) {
	struct line_entry *le = le_alloc(0); //FIXME: alloc failure
	le->tok = t;

	return le;
}

struct token token_list[] = {
	{ "PRINT",},
	{ "INPUT",},
	{ "NOT",},
	{ "IF",},
	{ "THEN",},
	{ "ELSE",},
	{ "OR",},
	{ "AND",},
	{ "ON",},
	{ "ERROR",},
	{ "GOTO",},
	{ "END",},
	{ "DEF",},
	{ "PROC",},
	{ "FN",},
	{ "ENDPROC",},
	{ "STATIC",},
	{ "FOR",},
	{ "TO",},
	{ "STEP",},
	{ "NEXT",},
	{ "REPEAT",},
	{ "UNTIL",},
	{ "WHILE",},
	{ "ENDWHILE",},
	{ "BREAK",},
	{ "CONTINUE",},
	{ "ABS",},
	{ "MOD",},
	{ "DIV",},
	{ "DIM",},
	{ "LIST",},
	{ "FORK",},
	{ "TRUE",},
	{ "FALSE",},
	{ "MACRO",},
	{ "ALLOC",},
	{ "\"", tokfn_string, print_string},
	{ "\r", tokfn_eol},
	{ ",",},

	/* Comparison operators */
	{ "<",},
	{ ">",},
	{ "<=",},
	{ ">=",},
	{ "<>",},

	/* Comparison & assignment */
	{ "=",}, /* exactly equal - perhaps need a '==' for equivalent? */

	{ "(",},
	{ ")",},

	/* Math operators */
	/* An = char after these makes them assign the lvalue into the first
	 * operand
	 */
	{ "*",},
	{ "/",},
	{ "+",},
	{ "-",}, /* negation / subtraction */
	{ "<<",},
	{ ">>",},

	{ "?",}, /* prefix: indirect u8       suffix: u8  variable */
	{ "%",}, /* prefix: indirect u16      suffix: u16 variable */
	{ "!",}, /* prefix: indirect u32      suffix: u32 variable */
	{ "$",}, /* prefix: indirect string   suffix: string variable */
	{ "#",}, /* prefix: indirect double   suffix: double variable */
	{ "^",}, /* prefix: indirection operator suffix: pointer variable */
	{ "|",},
	{ ":",},
	{ "~",},
	{ "&",},
	{ "@",}, /* prefix: system variables */
	{ "/*",}, /* need evaluator for comments */

	/* special cases in the lexer - NOT IMPLEMENTED YET */
	/* \ - line continuation */

	/* 0-9 - numbers */
	/* 0n (where n is one or more digits) - octal */
	/* 0xn (where n is one or more hex digits */
	/* 0bn (where n is one or more binary digits */

	/* Anything else non-matching - labels */
	{NULL},
};

void print_label(struct line_entry *le) {
	if(le->data)
		printf("{%s} ", (char *)le->data);
}

struct token tok_label = { "<label>", NULL, print_label};

struct tok_tree_entry {
	char c;
	struct tok_tree_entry *next;
	struct tok_tree_entry *children;
	struct token *tok;
};

struct tok_tree_entry *tok_tree = NULL;

/*
 * We assume the token being added is valid - this function is recursive and
 * so we dont know if we are being called initially or are already at some
 * stage in the process of adding a token
 */

int tok_add(struct tok_tree_entry **ptte, struct token *t, char *c) {
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
			if(!*c) /* Entry is a duplicate */
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

#define IS_LABEL(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

struct line_entry *extract_label(char **ps) {
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

struct line_entry *tokenise(struct tok_tree_entry *tok_tree, char *string) {
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

void tok_print_line(struct line_entry *le) {

	while(le) {
		if(le->tok->print)
			le->tok->print(le);
		else
			printf("%s", le->tok->name);
		le = le->next;
	}
	printf("\n");

}

/*
 * TODO: Add code to make a "flattened" tree, which can be parsed more quickly
 * As an optimisation, store "tree row length" and keep entries in alpha order,
 * allowing faster matching of tokens (binary search)
 */

int main (void) {
	struct token *t;
	struct line_entry *le;

	t = token_list;

	while(t->name) {
		if(tok_add(&tok_tree, t, t->name)) {
			printf("Failed whilst adding token: \"%s\"", t->name);
			exit(1);
		}
		t++;
	}

#if 0
	tokenise(tok_tree, "");
	tokenise(tok_tree, "\r");
	tokenise(tok_tree, "\n");
	tokenise(tok_tree, "\r\n");
	tokenise(tok_tree, "\n\r");
	tokenise(tok_tree, "\r\n ");
	tokenise(tok_tree, " ");
	tokenise(tok_tree, "L");
	tokenise(tok_tree, " L");
	tokenise(tok_tree, "  L");
	tokenise(tok_tree, "   L");
	tokenise(tok_tree, "L ");
	tokenise(tok_tree, "L  ");
	tokenise(tok_tree, "LABLE");
	tokenise(tok_tree, "5 PR\nOC");
	tokenise(tok_tree, "10 PR\n\nOC");
	tokenise(tok_tree, "15 PR\n\n\nOC");
	tokenise(tok_tree, "15 ENDENDPROCPROD\r\n");
	tokenise(tok_tree, "15 ENDPROCESSPROD\r\n");
	tokenise(tok_tree, "15 ENDPROCESSPROCPROD\r\n");
	tokenise(tok_tree, "FLOOBPRINT   PRINTIF  GOTO PROC ENDPFOG   END ENDPROC   ENDPROD goo \"blob\" ENDPROD");
	tokenise(tok_tree, "10 PRINT \"\"\r");
	tokenise(tok_tree, "10 PRINT \"Hello!\"PROC\r");
	tokenise(tok_tree, "10 PRINT \"Hello!\"PRALLS\r");
	tokenise(tok_tree, "10 PRINT \"Hello!\"BALLS\r");
	tokenise(tok_tree, "10 PRINT \" Hello!\"\r");
	tokenise(tok_tree, "20 INPUT A$\r");
	tokenise(tok_tree, "30 DEFPROCthingy(A$, THING%)\r");
	tokenise(tok_tree, "30 DEFPROCthingy(A$, THING%, \"cobbling\")\r");
	tokenise(tok_tree, "40 PRINT A$:PRINT THING%:GOTO out\r");
	tokenise(tok_tree, "50 ENDPROC\r");
	tokenise(tok_tree, "60 ENDPROCGOO");
#endif

	le = tokenise(tok_tree, "30 DEFPROCthingy(A$, THING%, \"cobbling\")\r");

	printf("------------------------\n");
	tok_print_line(le);

	return 0;
}
