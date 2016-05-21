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

//#define DEBUG_EXPR_STACK

void push(struct stack *s, struct line_entry *le) {
#ifdef DEBUG_EXPR_STACK
	printf("push(%08x) %d: ", s, s->sp);
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
	printf("pop (%08x) %d: ", s, s->sp);
	tok_print_one(le);
	printf("\n");
	} while (0);
#endif

	return s->le[s->sp];
}

struct line_entry *pop_nocheck(struct stack *s) {

	if(--s->sp >= 0)
		return s->le[s->sp];

	return NULL;
}

struct line_entry *peek(struct stack *s) {
	if(s->sp > 0)
		return s->le[s->sp-1];

	return NULL;
}

int get_prec(struct line_entry *a) {
	int t;

	if(!a)
		goto out;

	t = tokid(a);

	if(t == tokn_uminus || t == tokn_uplus)
		return 3;
	if(t == tokn_asterisk || t == tokn_slash)
		return 2;
	if(t == tokn_minus || t == tokn_plus)
		return 1;
	if(t == tokn_oparen)
		return -1;

	printf("Unknown Operator!\n");
	exit(1);

out:
	return 0;
}

int preceeds(struct line_entry *a, struct line_entry *b) {
	int pa = get_prec(a), pb = get_prec(b);

	return pa > pb;
}

void do_expression(struct stack *output, struct stack *operator);
void expression(void);
void expr_list(void);

/* Special case, replacement tokens for unary + and -
 * These are never emitted by the tokeniser, so it does not matter
 * that their names are not valid syntax
 */
struct token token_uplus  = {tokn_uplus, "u+"};
struct token token_uminus = {tokn_uminus, "u-"};

void factor(struct stack *output, struct stack *operator){

	/* Unary operators */
	if(tok_is(tokn_plus) || tok_is(tokn_minus)) {

		/* Promote token to a unary one */
		if(tok_is(tokn_plus))
			le->tok = &token_uplus;
		else
			le->tok = &token_uminus;

		if(!preceeds(le, peek(operator)))
			push(output, pop(operator));

		push(operator, le);

		next_le();
	}

	if(tok_is(tokn_plus) || tok_is(tokn_minus)) {
		printf("Syntax error!\n");
		exit(1);
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
		push(operator, le);

		next_le();

		do_expression(output, operator);

		expect(tokn_cparen);

		pop(operator); /* pop the open parentesis */
	}
	else if(tok_is(tokn_fn)) {
		struct line_entry *t = le;
		int n_params = 0;

		next_le();

		expect(tokn_label);

		if(tok_is(tokn_oparen)) {

			push(operator, le);

			next_le();

			do {
				do_expression(output, operator);
				n_params++;
			} while(accept(tokn_comma));

			expect(tokn_cparen);

			pop(operator); /* pop the open parentesis */
		}

		t->data.i = n_params;

		push(output, t);
	}
}

void term(struct stack *output, struct stack *operator) {

	factor(output, operator);

	while(tok_is(tokn_asterisk) || tok_is(tokn_slash)) {

		if(!preceeds(le, peek(operator)))
			push(output, pop(operator));

		push(operator, le);

		next_le();

		factor(output, operator);
	}
}

void do_expression(struct stack *output, struct stack *operator) {
	struct line_entry *t;

	term(output, operator);

	while(tok_is(tokn_plus) || tok_is(tokn_minus)) {

		if(!preceeds(le, peek(operator)))
			push(output, pop(operator));

		push(operator, le);

		next_le();
		term(output, operator);
	}

	/* Flush operator stack */
	while((t = peek(operator)) && tokid(t) != tokn_oparen)
		push(output, pop_nocheck(operator));

}

int eval(struct stack *o) {
	struct line_entry *t = pop(o);
	int i = tokid(t);

	if(i == tokn_label)
		return strtol(t->data.s, NULL, 10);
	else if(i == tokn_uplus) {
			return eval(o);
	}
	else if(i == tokn_uminus) {
			return -eval(o);
	}
	else if(i == tokn_plus) {
		return eval(o) + eval(o);
	}
	else if(i == tokn_minus) {
		int a = eval(o), b = eval(o);
		return b-a;
	}
	else if(i == tokn_asterisk) {
		int a = eval(o), b = eval(o);
		return b*a;
	}
	else if(i == tokn_slash) {
		int a = eval(o), b = eval(o);
		return b/a;
	}
	else if(i == tokn_fn) {
		int n = t->data.i;
		int r = 0;

		while(n) {
			r += eval(o); // Temporarily, lets just sum the params
			n--;
		}
		return r;
	}

	printf("Error: unknown operator\n");
	exit(1);
}

void expression() {
	struct stack output = {0}, operator = {0};

	/* Build RPN form of an expression */
	do_expression(&output, &operator);

	/* Display evaluated expression */
	if(peek(&output))
		printf("%d ", eval(&output));
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

	if(accept(tokn_label)) {
		if(accept(tokn_colon)) {
			emit("<label>:");
			goto out; /* Labels must be on their own on a line? */
		}
		else {
			emit("l-value = ");
			assign();
			expression();
		}
	}
	else if(accept(tokn_colon)) {
		; /* Skip leading : at beginning of lines */
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
