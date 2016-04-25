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
};

struct line_entry *le_alloc(int len) {
	struct line_entry *le = malloc(sizeof(*le)+len);

	if(len)
		le->data = le+1;

	return le;
}

struct line_entry *tok_string(struct token *t, char **ps) {
	static int in_string;
	char *s = *ps;
	char *dest;
	int len;
	struct line_entry *le;

	if(!in_string) {

		while(*s && *s != '"')
			s++;

		// if(!*s)  FIXME: check for end of line / non-string chars

		len = s-*ps;

		le = le_alloc(len+1); // FIXME: Check failure

		dest = (char *)le->data;
		memcpy(dest, *ps, len);
		dest[len] = 0;

		le->tok = t;

		printf("Found string: %s\n", dest);

		*ps = s;
	}
	else {
		printf("Found closing \"\n");

		le = le_alloc(0); // FIXME: Check failure

		le->tok = t;
	}

	in_string = !in_string;

	return le;
}

struct line_entry *tok_eol(struct token *t, char **ps) {
	char *s = *ps;
	struct line_entry *le = le_alloc(0);

	printf("Found EOL\n");

	/* We accept \r or \r\n as valid EOL so check for a \n */
	if(*s && *s == '\n')
		*ps = ++s;

	le->tok = t;

	return le;
}

struct line_entry *default_tok(struct token *t, char **ps) {
	struct line_entry *le = le_alloc(0);
	le->tok = t;

	printf("Token %s\n", t->name);

	return le;
}

struct token token_list[] = {
	{ "PRINT",},
	{ "IF",},
	{ "GOTO",},
	{ "INPUT",},
	{ "THEN",},
	{ "LIST",},
	{ "END",},
	{ "DEF",},
	{ "PROC",},
	{ "FN",},
	{ "ENDPROC",},
	{ "FOR",},
	{ "NEXT",},
	{ "REPEAT",},
	{ "UNTIL",},
	{ "DIM",},
	{ "\"", tok_string},
	{ "\r", tok_eol},
	{ ",",},
	{ "<",},
	{ ">",},
	{ "<=",},
	{ ">=",},
	{ "=",},
	{ "+",},
	{ "-",},
	{ "(",},
	{ ")",},
	{ "?",},
	{ "!",},
	{ "|",},
	{ "$",},
	{ "%",},
	{ "^",},
	{ ":",},
	{ "#",},
	NULL,
};

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
	struct tok_tree_entry *parent_tte = *ptte;

	/* Are we creating a new entry? */
	if(!*ptte) {
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

	while(parent_tte) {
		if(parent_tte->c == *c) { /* Entry exists */
			c++;
			if(!*c) /* Entry is a duplicate */
				return 1;
			else    /* Add new entry */
				return tok_add(&parent_tte->children, t, c);
		}

		if(!parent_tte->next) /* No existing entry, recursively add */
			return tok_add(&parent_tte->next, t, c);

		parent_tte = parent_tte->next;
	}
}

void test_tok(struct tok_tree_entry *tte, char *s) {

	while(tte) {
		printf("%c", tte->c);
		if(tte->c == *s) {
			s++;
			if(!*s) {
				printf("Found!\n");
				return;
			} else {
				tte = tte->children;
				printf(" --> ");
			}
		}
		else
			tte = tte->next;
	}

}

#define IS_WS(c) ((c)==' ' || (c)=='\t')

#define IS_LABEL(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

struct line_entry *extract_label(char **ps) {
	char *s = *ps;
	char *b = s;
	struct line_entry *le = le_alloc(0);

	while(*s && IS_LABEL(*s))
		s++;

	/* We cannot handle bad characters, skip them */
	if ( b == s ) {
		*ps = ++s;
		return 0;
	}


	{
		char *y = b;
		if(*y >= '0' && *y <= '9')
			printf("Number: ");  // Crude hack, no decimal point
		else
			printf("Label: ");
		while(y < s)
			printf("%c", *y++);
		printf("\n");
	}

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

	printf("Tokenise: %s\n", string);

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

		if(!tte)
			s = b; /* Backtrack */

		if (t) { /* Found a token */
			if(t->tok_func)
				le = t->tok_func(t, &s);
			else
				le = default_tok(t, &s);
		}
		else     /* Non-token thing found */
			le = extract_label(&s);

		if(le) {
			*pl = le;
			pl = &le->next;
		}
	}

	return l;
}

/*
 * TODO: Add code to make a "flattened" tree, which can be parsed more quickly
 * As an optimisation, store "tree row length" and keep entries in alpha order,
 * allowing faster matching of tokens (binary search)
 */

int main (void) {
	struct token *t;

	t = token_list;

	while(t->name) {
		printf("Adding token: '%s' %s.\n", t->name, tok_add(&tok_tree, t, t->name)?"Failed":"OK");
		t++;
	}

	t = token_list;

	while(t->name) {
		printf("Lookup: %s ... ", t->name);
		test_tok(tok_tree, t->name);
		t++;
	}

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
	tokenise(tok_tree, "FLOOBPRINT   PRINTIF  GOTO PROC ENDPFOG   END ENDPROC   ENDPROD goo \"blob\" ENDPROD");
	tokenise(tok_tree, "10 PRINT \"Hello!\"PROC\r");
	tokenise(tok_tree, "10 PRINT \"Hello!\"PRALLS\r");
	tokenise(tok_tree, "10 PRINT \"Hello!\"BALLS\r");
	tokenise(tok_tree, "10 PRINT \" Hello!\"\r");
	tokenise(tok_tree, "20 INPUT A$\r");
	tokenise(tok_tree, "30 DEFPROCthingy(A$, THING%)\r");
	tokenise(tok_tree, "40 PRINT A$:PRINT THING%:GOTO out\r");
	tokenise(tok_tree, "50 ENDPROC\r");
	tokenise(tok_tree, "60 ENDPROCGOO");

	return 0;
}
