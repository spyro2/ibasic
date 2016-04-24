#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


int tok_string(char **ps) {
	char *s = *ps;
	static int in_string;

	if(!in_string) {
		in_string = !in_string;

		printf("Found string: ");
		while(*s && *s != '"')
			printf("%c", *s++);
		printf("\n");

		*ps = s;
	}
	else
		printf("Found closing \"\n");

	return 0;
}

int tok_eol(char **ps) {
	char *s = *ps;

	printf("Found EOL\n");

	/* We accept \r or \r\n as valid EOL so check for a \n */
	if(*s && *s == '\n')
		*ps = ++s;

	return 0;
}

struct token {
	char *name;
	int (*tok_func)(char **s);
};

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
	{ "PRINTER",},
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

char *extract_label(char *b, char **ps) {
	char *s = b;
	char *l = s;

	while(*s && IS_LABEL(*s))
		s++;

	/* We cannot handle bad characters, skip them */
	if ( l == s ) {
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

	return l; // This is crap, but non-zero is enough for now.
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

void tokenise(struct tok_tree_entry *tok_tree, char *string) {
	struct tok_tree_entry *tte;
	struct token *t = NULL;
	char *s = string, *b;

	/* b points to the character after the last successfully tokenised thing */
	b = s;

	while(*s) {
		/* Skip whitespace */
		while(IS_WS(*s)) {
			s++;
			b = s;
		}
		if(!*s)
			break;

		tte = tok_tree;

//		printf("Search: %s\n", b);
		while(tte) {
//			printf("%c %c:\n", *s, tte->c);
			if(tte->c == *s) {
				s++;

				if(tte->tok) { /* potentially found token */
					t = tte->tok;
					b = s;
				}

				if(!*s || IS_WS(*s))
					break;
				else
					tte = tte->children;
			}
			else
				tte = tte->next;
		}

		if (t) { /* Token found */
			if(t->tok_func)
				t->tok_func(&s);
			else
				printf("Token %s\n", t->name);
			t = NULL;
		}
		else {  /* Non-token thing found */
			/* For now, pretend all unidentifiable things are labels */
			if(!extract_label(b, &s))
				printf("Skipping bad char\n");

			b = s;
		}

	}
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
	tokenise(tok_tree, "FLOOBPRINT   PRINTIF  GOTO PROC ENDPFOG   END ENDPROC   ENDPROD");
	tokenise(tok_tree, "10 PRINT \"Hello!\"\r");
	tokenise(tok_tree, "20 INPUT A$\r");
	tokenise(tok_tree, "30 DEFPROCthingy(A$, THING%)\r");
	tokenise(tok_tree, "40 PRINT A$:PRINT THING%:GOTO out\r");
	tokenise(tok_tree, "50 ENDPROC\r");

	return 0;
}
