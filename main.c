#include <stdio.h>

enum tok_type {
	TOK_COMMENT = 0,
	TOK_STRING  = 1,
	TOK_CHAR    = 2,
	TOK_INT     = 3,
	TOK_FLOAT   = 4,
	TOK_LABEL   = 5,
	TOK_STATEMENT = 6,
	TOK_OPERATOR = 7,
	TOK_END = -1,
};

union content {
	char c;
	int i;
	float f;
	char *ps;
	int *pi;
	float *pf;
}

struct token {
	enum tok_type type;
	union content
	void *content;
};

struct line {
	int number;		// Line number for GOTO
	char *source_line;
	struct token *token;    // pointer to list of tokens that make up the line
};

int tok_add_to_tree(struct token *t) {
	add token to tree type parser.
}

int tok_create_tree() {
	for_each_token() {
		tok_add_to_tree(token);
	}
	return 0;
}

struct token *tok_descend(char *c) {
	if (is_ok(**c))
		return descend(c++);

	if(valid_token)
		malloc(token);
		fill_out_token;
		return struct_token;

	return NULL;
}

int tokenise_one(struct line *l, struct token *tlist) {
	char *c = *p;
	struct token *t;

	do {
		t = tok_descend(c);

		if(t)
			list_add_tail(l->tlist, t);
			
		else
			return -ERR;
	
	while(t->type != TOK_END);
		
}

struct line *tokenise_next_line() {

	struct line *l;
	char *pos, *eol;

	l = (struct line *) malloc(sizeof(*l));

	l->source_line = get_line_from_input();

	do {
		if(!tokenise_one(l, token)
			printf("Syntax error\n");

	} while (pos < eol);

	return line;
}


int main (void) {

	tok_create_tree();
	
	while(!eof) {
		line = tokenise_next_line();
		parse (line);
	}

	return 0;
}

