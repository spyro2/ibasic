#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tokeniser.h"
#include "colours.h"

static int fd;
struct line_entry *le;

void print_current_token(char *s) {
	printf("%s", s);
        tok_print_one(le);
        printf(ANSI_RESET);
}

void next_le(void) {
	/* Skip comments - This may have implications for writing a
	 * pretty-printer.
	 */
	do {
		le = get_next_le(fd, NULL);
	} while (le->tok->id == tokn_comment);

#if 0
	printf(ANSI_GREEN);
	tok_print_one(le);
	printf(ANSI_RESET);
#endif
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

static int indent_l = 0;

#define emit_noindent(a) printf((a))

#define indent \
	do { \
		int i; \
		for(i = 0 ; i < indent_l ; i++) \
			printf("\t"); \
	} while(0)

#define emit(a) \
	do { \
	indent; \
	emit_noindent((a)); \
	} while(0)

#define emit_i(a) \
	do { \
	emit((a)); \
	indent_l++; \
	} while(0)

#define emit_o(a) \
	do { \
	indent_l--; \
	emit((a)); \
	} while(0)

#define MAX_EXPR_STACK 64

struct stack {
	struct line_entry *le[MAX_EXPR_STACK];
	int sp;
};

#define tokid(a) (a)->tok->id

void push(struct stack *s, struct line_entry *le) {
#ifdef DEBUG_EXPR_STACK
	printf("push(%08x): ", s);
	if((tokid(le) == tokn_plus || tokid(le) == tokn_minus) && le->data)
		printf("u");
	tok_print_one(le);
	printf("\n");
#endif

	s->le[s->sp++] = le;
	if (s->sp >= MAX_EXPR_STACK) {
		printf("expression stack overflow\n");
		exit(1);
	}
}

struct line_entry *pop(struct stack *s) {

	if(--s->sp < 0) {
		printf("expression stack underflow\n");
		exit(1);
	}

#ifdef DEBUG_EXPR_STACK
	do {
	struct line_entry *le;
	le = s->le[s->sp];
	printf("pop (%08x): ", s);
	if((tokid(le) == tokn_plus || tokid(le) == tokn_minus) && le->data)
		printf("u");
	tok_print_one(le);
	printf("\n");
	} while (0);
#endif

	return s->le[s->sp];
}

struct line_entry *peek(struct stack *s) {
	if(s->sp > 0)
		return s->le[s->sp-1];

	return NULL;
}

int preceeds(struct line_entry *a, struct line_entry *b) {
	if(!b)
		return 1;
	if(a->data)
		return 1;
	if(tokid(a) == tokn_asterisk || tokid(a) == tokn_slash)
		if(tokid(b) == tokn_plus || tokid(b) == tokn_minus)
			return 1;
	return 0;
}

void do_expression(struct stack *output, struct stack *operator);
void expression(void);
void expr_list(void);

void factor(struct stack *output, struct stack *operator){

	/* Unary operators */
	if(tok_is(tokn_plus) || tok_is(tokn_minus)) {
		struct line_entry *t;

		t = peek(operator);

		if(t && preceeds(le, t) && tokid(t) != tokn_oparen)
			push(output, pop(operator));

		le->data = (void *)1;

		push(operator, le);

		next_le();
	}

	if(tok_is(tokn_label)) {
		push(output, le);

		next_le();
	}
#if 0
	/* numbers and labels are all the same to the tokeniser right now */
	else if (accept(number)) {
		;
	}
#endif
	else if(tok_is(tokn_oparen)) {
	struct line_entry *t;
		push(operator, le);
		next_le();
		do_expression(output, operator);
		expect(tokn_cparen);
		while((t = peek(operator))) {
			if(tokid(t) == tokn_oparen) {
				pop(operator);
				break;
			}
			push(output, pop(operator));
		}
	}
	else if(tok_is(tokn_fn)) {
		struct line_entry *tt = le;
		next_le();
		expect(tokn_label);
		if(tok_is(tokn_oparen)) {
			struct line_entry *t;
			push(operator, le);
			next_le();
			do {
				do_expression(output, operator);
				if(tok_is(tokn_comma)) {
					push(output,le);
					while((t = peek(operator))) {
						if(tokid(t) == tokn_oparen) {
							break;
						}
						push(output, pop(operator));
					}
				}
			} while(accept(tokn_comma));
			expect(tokn_cparen);
			while((t = peek(operator))) {
				if(tokid(t) == tokn_oparen) {
					pop(operator);
					break;
				}
				push(output, pop(operator));
			}
		}
		push(output, tt);
	}
}

void term(struct stack *output, struct stack *operator) {
	factor(output, operator);
	while(tok_is(tokn_asterisk) || tok_is(tokn_slash)) {

		struct line_entry *t = peek(operator);

		if(!preceeds(le, t) && tokid(t) != tokn_oparen)
			push(output, pop(operator));

		push(operator, le);

		next_le();
		factor(output, operator);
	}
}

void do_expression(struct stack *output, struct stack *operator) {
	struct line_entry *t;
//	static int ident;
//	int i;

//ident++;
//for(i = 0 ; i < ident ; i++)
//	printf("#");
//printf("\n");

	term(output, operator);

	while(tok_is(tokn_plus) || tok_is(tokn_minus)) {

		t = peek(operator);

		if(!preceeds(le, t) && tokid(t) != tokn_oparen)
			push(output, pop(operator));

		push(operator, le);

		next_le();
		term(output, operator);
	}

//ident--;
//for(i = 0 ; i < ident ; i++)
//	printf("#");
//printf("\n");
}

void expression() {
	struct line_entry *t;
	struct stack output = {0}, operator = {0};

	do_expression(&output, &operator);

	while((t = peek(&operator)))
		push(&output, pop(&operator));

//printf("result: \n");
	while((t = peek(&output))) {
		if((tokid(t) == tokn_plus || tokid(t) == tokn_minus) && t->data)
			printf("u");
		tok_print_one(t);
		if(tokid(t) == tokn_fn)
			tok_print_one(t->next);

		pop(&output);
	}
}

void condition(void) {
	expression();

	if(tok_is(tokn_lt) || tok_is(tokn_gt) ||
	   tok_is(tokn_le) || tok_is(tokn_ge) ||
	   tok_is(tokn_ne) || tok_is(tokn_eq)) {
		emit_noindent("<cond> ");
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
	int d = 0;

	do {
		if(d && !tok_is(tokn_eol))
			emit_noindent("\n");

		statement();

	} while((d = accept(tokn_colon)));
}

void expr_list(void) {
	do {
		expression();
		if(tok_is(tokn_comma))
			emit_noindent(", ");
	} while(accept(tokn_comma));
}

void input_param_list(void) {
	do {
		if(accept(tokn_return))
			emit_noindent("RETURN ");

		expect(tokn_label);

		emit_noindent("<label>");

		if(tok_is(tokn_comma))
			emit_noindent(", ");

	} while(accept(tokn_comma));
}

void definition(void) {
	emit_i("DEF ");
	if(accept(tokn_proc)) {

		emit_noindent("PROC");

		expect(tokn_label);

		emit_noindent("<label>");

		if(accept(tokn_oparen)) {
			emit_noindent(" ( ");
			input_param_list();
			expect(tokn_cparen);
			emit_noindent(" ) ");
		}

		emit_noindent(" {\n");

		if(accept(tokn_colon)) {
			statement_list(); /* What about IF and ENDPROC? */
			emit_noindent("\n");
		}

		expect(tokn_eol);

		while(!accept(tokn_endproc))
			line();

		emit_o("} ENDPROC\n");

	}
	else if(accept(tokn_fn)) {

		emit_noindent("FN");

		expect(tokn_label);

		emit_noindent("<label>");

		if(accept(tokn_oparen)) {
			emit_noindent(" ( ");
			input_param_list();
			expect(tokn_cparen);
			emit_noindent(" )");
		}

		emit_noindent(" {\n");

		if(accept(tokn_colon)) {
			statement_list(); /* What about IF and = ? */
			emit_noindent("\n");
		}

		expect(tokn_eol);

		while (!accept(tokn_eq))
			line(); /* What about IF and = ? */

		emit("RETURN ");

		expression();

		emit_noindent("\n");
		emit_o("} ENDFN\n");
	}

	expect(tokn_eol);
}

void statement(void) {
	if(accept(tokn_if)) {
		emit("IF (");

		condition();

		emit_noindent(") {\n");
		indent_l++;

		if(accept(tokn_then) && accept(tokn_eol)) {
			while(!tok_is(tokn_endif)) {

				line();

				if(accept(tokn_else)) {
					emit_o("}\n");
					emit_i("ELSE {\n");

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

			if (accept(tokn_else)) {
				emit_noindent("\n");
				emit_o("}\n");
				emit_i("ELSE {\n");

				statement_list();
			}

			emit_noindent("\n");

			accept(tokn_endif);

		}
		emit_o("} ENDIF");
	}
	else if(accept(tokn_case)) {
		emit_i("CASE ");

		expression();
		expect(tokn_of);
		emit_noindent("OF {\n");
		expect(tokn_eol);

		while(!tok_is(tokn_endcase)) {
			if(accept(tokn_when)) {
				emit_i("WHEN ");
				expr_list();
				emit_noindent(" {\n");
			}
			else {
				expect(tokn_otherwise);
				emit_i("OTHERWISE {\n");
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
				emit_noindent("\n");
			}
			emit_o("}\n");
		}
		expect(tokn_endcase);
		emit_o("} ENDCASE");

	}
	else if (accept(tokn_proc)) {
		emit("PROC");
		expect(tokn_label);
		emit_noindent("<label>");
		if(accept(tokn_oparen)) {
			emit_noindent(" ( ");
			expr_list();
			expect(tokn_cparen);
			emit_noindent(")");
		}
	}
	else if (accept(tokn_fn)) {
		emit("FN");
		expect(tokn_label);
		emit_noindent("<label>");
		if(accept(tokn_oparen)) {
			emit_noindent(" ( ");
			expr_list();
			expect(tokn_cparen);
			emit_noindent(" )");
		}
	}
	else if (accept(tokn_repeat)) {
		emit_i("REPEAT {\n");

		if(accept(tokn_colon))
			statement_list();
		else
			expect(tokn_eol);

		while(!accept(tokn_until))
			line();

		emit_o("} UNTIL ( ");

		condition();

		emit_noindent(")");
	}
	else if (accept(tokn_while)) {
		emit_i("WHILE ( ");

		condition();

		emit_noindent(") {\n");

		if(accept(tokn_colon))
			statement_list();
		else 
			expect(tokn_eol);

		while(!accept(tokn_endwhile))
			line();

		emit_o("} ENDWHILE");
	}
	else if (accept(tokn_goto)) {
		emit("GOTO ");
		expect(tokn_label);
		emit_noindent("<label>");
	}
	else if(accept(tokn_print)) {
		emit("PRINT");
		if(!tok_is(tokn_eol) && !tok_is(tokn_colon)) {
			emit_noindent(" {\n");
			indent_l++;
			do {
				if(accept(tokn_string))
					emit("{string}");
				else {
					indent;
					expression();
				}
				emit_noindent("\n");
			} while(accept(tokn_semicolon) && !tok_is(tokn_eol) && !tok_is(tokn_colon));
			emit_o("}");
		}
	}
	else if(accept(tokn_label)) {
		emit("l-value = ");
		assign();
		expression();
	}
	else if (accept(tokn_end)) {
		emit("END");
	}
}

void line(void) {
	if(tok_is(tokn_eol))
		goto out; /* Empty line */

	if(accept(tokn_colon)) {
		; /* Skip leading : at beginning of lines */
	}
	else if(accept(tokn_label)) {
		if(!accept(tokn_colon)) {
			emit("l-value = ");
			assign();
			expression();
		}
		else {
			emit("<label>:");
			goto out;
		}
	}
	else if(accept(tokn_library)) {
		emit("LIBRARY ");
		expect(tokn_string);
		emit_noindent("<filename>");
	}
	else if(tok_is(tokn_static) || tok_is(tokn_global) || tok_is(tokn_const)) {
		if(tok_is(tokn_static))
			emit("STATIC ");
		if(tok_is(tokn_global))
			emit("GLOBAL ");
		if(tok_is(tokn_const))
			emit("CONST ");

		next_le();

		expect(tokn_label);

		emit_noindent("<label>");

		if(accept(tokn_eq)) {
			emit_noindent(" = ");
			expression();
		}
	}

	statement_list();

out:
	emit_noindent("\n");
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
