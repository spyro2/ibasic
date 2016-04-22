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
	{ ":",},
	{ ":",},
/*
	{ "IF",},
	{ "GOTO",},
	{ "INPUT",},
	{ "THEN",},
	{ "LIST",},
	{ "END",},
	{ "DEF",},
	{ "PROC",},
	{ "FN",},
*/
	NULL,
};

struct tok_tree_entry {
	char c;
	struct tok_tree_entry *next;
	struct tok_tree_entry *children;
	struct token *tok;
};

struct tok_tree_entry *tok_tree = NULL;

int tok_add(struct tok_tree_entry **ptte, struct token *t, char *c) {
	//is_tok_valid();

	if(!*ptte) {
		struct tok_tree_entry *tte_new;
		int r = 0;

		tte_new = calloc(1, sizeof(*tte_new));
		if(!tte_new)
			return -ENOMEM;

		tte_new->c = *c;

//		printf("Alloc new tok: %c @ %08x\n", *c, tte_new);

		c++;

		if(!*c)
			tte_new->tok = t;
		else
			r = tok_add(&tte_new->children, t, c);


		if(r == -ENOMEM) {
			free(tte_new);
		}
		else {
//			printf(" Assign %08x -> Addr %08x\n", tte_new, ptte);
			*ptte = tte_new;
		}

		return r;

	}

	struct tok_tree_entry *parent_tte = *ptte;

	while(parent_tte) {
//		printf("Scanning: '%c'\n", parent_tte->c);
		if(parent_tte->c == *c) {
//			printf("Descend\n");
			c++;
			if(!*c) {
//				printf("Dup!\n");
				return 1;
			} else
				return tok_add(&parent_tte->children, t, c);
		}

		if(!parent_tte->next) {
//			printf("Add peer\n");
			return tok_add(&parent_tte->next, t, c);
		}

		parent_tte = parent_tte->next;
	}
}

int main (void) {
	struct token *t;

	t = token_list;

//	printf("Tree root: %08x\n", &tok_tree);

	while(t->name) {
		printf("Adding token: '%s' %s.\n", t->name, tok_add(&tok_tree, t, t->name)?"Failed":"OK");
		t++;
	}

	return 0;
}
