#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

struct token {
	char *name;
	int (*tok_func)(void);
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

	return 0;
}
