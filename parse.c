#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tokeniser.h"

static int fd;
struct line_entry *le;

void next_le(void) {
	/* Skip comments - This may have implications for writing a
	 * pretty-printer.
	 */
	do {
		le = get_next_le(fd, NULL);
	} while (le->tok->id == tokn_comment);

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

void expression(void);

void factor(void){
	if(accept(tokn_label)) {
		;
	}
#if 0
	/* numbers and labels are all the same to the tokeniser right now */
	else if (accept(number)) {
		;
	}
#endif
	else if(accept(tokn_oparen)) {
		expression();
		expect(tokn_cparen);
	}
}

void term(void) {
	factor();
	while(tok_is(tokn_asterisk) || tok_is(tokn_slash)) {
		next_le();
		factor();
	}
}

void expression(void) {
	if(tok_is(tokn_plus) || tok_is(tokn_minus))
		next_le();
	term();
	while(tok_is(tokn_plus) || tok_is(tokn_minus)) {
		next_le();
		term();
	}
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

void assign(void) {
	expect(tokn_eq); /* FIXME: assignment operator*/
}


void line(void);
void statement(void);

void statement_list(void) {
	do {
		statement();
	} while(accept(tokn_colon));
}

void expr_list(void) {
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

		if(accept(tokn_colon))
			statement_list(); /* What about IF and ENDPROC? */

		expect(tokn_eol);

		while(!accept(tokn_endproc))
			line();

	}
	else if(accept(tokn_fn)) {

		expect(tokn_label);

		if(accept(tokn_oparen)) {
			input_param_list();
			expect(tokn_cparen);
		}

		if (accept(tokn_colon))
			statement_list(); /* What about IF and = ? */

		expect(tokn_eol);

		while (!accept(tokn_eq))
			line(); /* What about IF and = ? */

		expression();
	}

	expect(tokn_eol);
}

void statement(void) {
	if(accept(tokn_if)) {

		condition();

		if(accept(tokn_then) && accept(tokn_eol)) {
			while(!tok_is(tokn_endif)) {

				line();

				if(accept(tokn_else)) {

					if(tok_is(tokn_if))
						line();
					else
						expect(tokn_eol);

					while (!tok_is(tokn_endif))
						line();
				}

			}

			expect(tokn_endif);
		}
		else {
			statement_list();

			if (accept(tokn_else))
				statement_list();

			accept(tokn_endif);
		}
	}
	else if(accept(tokn_case)) {

		expression();
		expect(tokn_of);
		expect(tokn_eol);

		while(!tok_is(tokn_endcase)) {
			if(accept(tokn_when)) {
				expr_list();
			}
			else {
				expect(tokn_otherwise);
			}

			if(accept(tokn_eol)) {
				while(!(tok_is(tokn_when) ||
				        tok_is(tokn_otherwise) ||
				        tok_is(tokn_endcase))) {
					line();
				}
			}
			else {
				expect(tokn_colon);
				statement_list();
				expect(tokn_eol);
			}
		}
		expect(tokn_endcase);

	}
	else if (accept(tokn_proc)) {
		expect(tokn_label);
		if(accept(tokn_oparen)) {
			expr_list();
			expect(tokn_cparen);
		}
	}
	else if (accept(tokn_fn)) {
		expect(tokn_label);
		if(accept(tokn_oparen)) {
			expr_list();
			expect(tokn_cparen);
		}
	}
	else if (accept(tokn_repeat)) {

		if(accept(tokn_colon))
			statement_list();
		else
			expect(tokn_eol);

		while(!accept(tokn_until))
			line();

		condition();
	}
	else if (accept(tokn_while)) {

		condition();

		if(accept(tokn_colon))
			statement_list();
		else 
			expect(tokn_eol);

		while(!accept(tokn_endwhile))
			line();
	}
	else if (accept(tokn_goto)) {
		expect(tokn_label);
	}
	else if(accept(tokn_print)) {
		if(!tok_is(tokn_eol) && !tok_is(tokn_colon)) {
			do {
				if(accept(tokn_string))
					;
				else {
					indent;
					expression();
				}
			} while(accept(tokn_semicolon) && !tok_is(tokn_eol) && !tok_is(tokn_colon));
		}
	}
	else if(accept(tokn_label)) {
		assign();
		expression();
	}
	else if (accept(tokn_end)) {
		;
	}
}

void line(void) {
	if(tok_is(tokn_eol))
		goto out; /* Empty line */

	if(accept(tokn_label)) {
		if(!accept(tokn_colon)) {
			assign();
			expression();
			if(accept(tokn_colon) && !tok_is(tokn_eol)) {
				statement_list();
			}
		}
	}
	else if(accept(tokn_library)) {
		expect(tokn_string);
		if(accept(tokn_colon) && !tok_is(tokn_eol)){
			statement_list();
		}
	}
	else if(tok_is(tokn_static) || tok_is(tokn_global) || tok_is(tokn_const)) {
		next_le();
		expect(tokn_label);
		if(accept(tokn_eq))
			expression();

		if(accept(tokn_colon) && !tok_is(tokn_eol)) {
			statement_list();
		}
	}
	else {
		statement_list();
	}

out:
	expect(tokn_eol);
}

void toplevel_line(void) {

	if (accept(tokn_def)) {
		definition();
	}
	else {
		line();
	}
}

int parse (int fd) {

	next_le();
	while(1) {
		toplevel_line();
	}

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
