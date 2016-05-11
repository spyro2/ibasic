#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tokeniser.h"

static int fd;
struct line_entry *le;

void next_le(void) {
	le = get_next_le(fd, NULL);
	tok_print_one(le);
}

int tok_is(enum tokid id) {
	return le->tok->id == id ? 1 : 0;
}

int accept(enum tokid id) {

	if (le->tok->id == id) {
		next_le();
		return 1;
	}

	return 0;
}

int expect(enum tokid id) {
	if (accept(id))
		return 1;

	printf("Unexpected token: %s\n", le->tok->name?le->tok->name:"Unknown");
	printf("Expected: %d (%s)\n", id, tok_from_id(id)?tok_from_id(id)->name:"null"); // FIXME: null deref
	exit(1);

	return 0;
}

void expression(void) {
	accept(tokn_label);
}

void condition(void) {
	expression();

	if(tok_is(tokn_lt) || tok_is(tokn_gt) ||
	   tok_is(tokn_le) || tok_is(tokn_ge) ||
	   tok_is(tokn_ne) || tok_is(tokn_eq)) {
		next_le();
		expression();
	}
}

void line(void);
void statement(void);

void statement_list(void) {
	do {
		statement();
	} while(accept(tokn_colon));
}

void param_list(void) {
	do {
		expression();
	} while(accept(tokn_comma));
}

void input_param_list(void) {
	do {
		accept(tokn_return);
		expect(tokn_label);
	} while(accept(tokn_comma));
}

void definition(void) {
	if(accept(tokn_proc)) {
		expect(tokn_label);
		if(accept(tokn_oparen)) {
			input_param_list();
			expect(tokn_cparen);
		}
		/* FIXME: allow statements following DEF PROCfoo ? */
		while(!tok_is(tokn_endproc))
			line(); /* FIXME: nested def should not happen */
		expect(tokn_endproc);
	}
	else if(accept(tokn_fn)) {
		expect(tokn_label);
		if(accept(tokn_oparen)) {
			input_param_list();
			expect(tokn_cparen);
		}
		/* FIXME: allow statements following DEF FNfoo ? */
		while(!tok_is(tokn_eq))
			line(); /* FIXME: nested def should not happen */
		expect(tokn_eq);
		expression();
	}
}

void statement(void) {
	if(accept(tokn_if)) {

		condition();

		if(accept(tokn_then) && accept(tokn_eol)) {
			do {
				line();

				if(accept(tokn_else)) {
					do {
						line();
					} while (!tok_is(tokn_endif));
				}

			} while(!tok_is(tokn_endif));

			expect(tokn_endif);
		}
		else {
			statement_list();

			if (accept(tokn_else))
				statement_list();

		}
	}
/*
	else if(tok_is(tokn_else)) {
		printf("Unexpected ELSE\n");
		exit(1);
	}
	else if(tok_is(tokn_then)) {
		printf("Unexpected THEN\n");
		exit(1);
	}
*/
	else if(accept(tokn_case)) {

		expression();
		expect(tokn_of);
		expect(tokn_eol);

		do {
			if(accept(tokn_when)) {
				do {
					expression();
				} while(accept(tokn_comma));
			}
			else {
				/* FIXME: only one OTHERWISE allowed */
				/* Also, WHEN after OTHERWISE is pointless */
				expect(tokn_otherwise);
			}

			if(accept(tokn_eol)) {
				do {
					line();
				} while(!(tok_is(tokn_when) ||
				          tok_is(tokn_otherwise) ||
				          tok_is(tokn_endcase)));
			}
			else {
				expect(tokn_colon);
				statement_list();
				expect(tokn_eol);
			}

		} while(!tok_is(tokn_endcase));

		expect(tokn_endcase);
	}
	else if(accept(tokn_print)) {
		do {
			if(!accept(tokn_string))
				expression();
		} while(accept(tokn_semicolon));
	}
}

void assign(void) {
	expect(tokn_eq); /* FIXME: assignment operator*/
}

void line(void) {

	if(accept(tokn_label)) {
		if(!accept(tokn_colon)) {
			assign();
			expression();
			if(accept(tokn_colon))
				statement_list();
		}
	}
	else if (accept(tokn_def)) {
		definition();
	}
	else {
		statement_list();
	}
	expect(tokn_eol);
}


int parse (int fd) {

	next_le();
	do {
		line();
	} while(!tok_is(tokn_end));

	return 0;
}



int main(void) {

	tokeniser_init();

	fd = open("test.bas", O_RDONLY);
	if(fd == -1) {
		printf("Couldnt open file\n");
		exit(1);
	}

	parse(fd);

	return 0;
}
