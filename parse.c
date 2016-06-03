#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tokeniser.h"
#include "stack.h"
#include "expression.h"
#include "ast.h"

#include "colours.h"

//#define PRINT_NEXT_TOKEN
//#define PRETTYPRINT

static int fd;
struct token *tok = NULL;

void print_current_token(char *s) {
	printf("%s", s);
        tok_print_one(tok);
        printf(ANSI_RESET);
}

int tok_is(enum tokid id) {
	return tok->sym->id == id ? 1 : 0;
}

void next_token(void) {
	int skip;

	if(tok)
		tok_put(tok);

	do {
		tok = get_next_token(fd);

		tok_get(tok);

		/* Skip comments - This may have implications for writing a
		 * pretty-printer.
		 */

		if((skip = tok_is(tokn_comment)))
			tok_put(tok);

	} while (skip);

#ifdef PRINT_NEXT_TOKEN
	printf(ANSI_GREEN);
	tok_print_one(tok);
	printf(ANSI_RESET);
#endif
}

int accept(enum tokid id) {

	if (tok->sym->id == id) {
		next_token();
		return 1;
	}

	return 0;
}

int expect(enum tokid id) {
	if (accept(id))
		return 1;

	printf("Unexpected token: %s\n", tok->sym->name?tok->sym->name:"Unknown");
	printf("Expected: %d (%s)\n", id, sym_from_id(id)?sym_from_id(id)->name:"null"); // FIXME: null deref
	exit(1);

	return 0;
}

static int indent_l = 0;

#ifdef PRETTYPRINT
#define emit_noindent(a) printf((a))

#define indent \
	do { \
		int i; \
		for(i = 0 ; i < indent_l ; i++) \
			printf("\t"); \
	} while(0)
#else
#define emit_noindent(a)
#define indent
#endif

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


int get_precedence(struct token *a) {

	if(!a)
		goto out;

	switch (tokid(a)) {
		case tokn_uminus:
		case tokn_uplus:
			return 3;
		case tokn_asterisk:
		case tokn_slash:
			return 2;
		case tokn_minus:
		case tokn_plus:
			return 1;
		case tokn_oparen:
			return -1;
		default:
			printf("Unknown Operator!\n");
			exit(1);
	}

out:
	return 0;
}

int preceeds(struct token *a, struct token *b) {
	int pa = get_precedence(a), pb = get_precedence(b);

	return pa > pb;
}

void do_expression(struct stack *output, struct stack *operator);
struct ast_entry *expression(void);
void expr_list(void);

/* Special case, replacement tokens for unary + and -
 * These are never emitted by the tokeniser, so it does not matter
 * that their names are not valid syntax
 */
struct symbol sym_uplus  = {tokn_uplus, "u+"};
struct symbol sym_uminus = {tokn_uminus, "u-"};

void factor(struct stack *output, struct stack *operator){

	/* Unary operators */
	if(tok_is(tokn_plus) || tok_is(tokn_minus)) {
		tok_get(tok);

		/* Promote token to a unary one */
		if(tok_is(tokn_plus))
			tok->sym = &sym_uplus;
		else
			tok->sym = &sym_uminus;

		if(!preceeds(tok, peek(operator)))
			push(output, pop(operator));

		push(operator, tok);

		next_token();
	}

	if(tok_is(tokn_plus) || tok_is(tokn_minus)) {
		printf("Syntax error!\n");
		exit(1);
	}

	if(tok_is(tokn_label) || tok_is(tokn_value)) {
		tok_get(tok);
		push(output, tok);

		next_token();
	}
	else if(tok_is(tokn_oparen)) {
		tok_get(tok);
		push(operator, tok);

		next_token();

		do_expression(output, operator);

		expect(tokn_cparen);

		tok_put(pop(operator)); /* pop the open parentesis */
	}
	else if(tok_is(tokn_fn)) {
		struct token *t = tok_get(tok), *tn;
		int n_params = 0;

		next_token();

		tn = tok_get(tok);
		expect(tokn_label);

		if(tok_is(tokn_oparen)) {
			tok_get(tok);
			push(operator, tok);

			next_token();

			do {
				do_expression(output, operator);
				n_params++;
			} while(accept(tokn_comma));

			expect(tokn_cparen);

			tok_put(pop(operator)); /* pop the open parentesis */
		}

		t->val->data.i = n_params;

		push(output, tn); /* Push the function name */
		push(output, t);

	}
}

void term(struct stack *output, struct stack *operator) {

	factor(output, operator);

	while(tok_is(tokn_asterisk) || tok_is(tokn_slash)) {
		tok_get(tok);

		if(!preceeds(tok, peek(operator)))
			push(output, pop(operator));

		push(operator, tok);

		next_token();

		factor(output, operator);
	}
}

void do_expression(struct stack *output, struct stack *operator) {
	struct token *t;

	term(output, operator);

	while(tok_is(tokn_plus) || tok_is(tokn_minus)) {
		tok_get(tok);

		if(!preceeds(tok, peek(operator)))
			push(output, pop(operator));

		push(operator, tok);

		next_token();

		term(output, operator);
	}

	/* Flush operator stack */
	while((t = peek(operator)) && tokid(t) != tokn_oparen)
		push(output, pop_nocheck(operator));

}

struct ast_entry *expression() {
	struct stack output = {{0}}, operator = {{0}};
	struct token *t;

	/* Build RPN form of an expression */
	do_expression(&output, &operator);

	/* Display evaluated expression */
#ifdef PRETTYPRINT
	print_expression(&output);
#endif

	/* Free tokens from expression */
	while((t = peek(&output))) {
		tok_put(pop_nocheck(&output));
	}

	return NULL; /* For now */
}

void condition(void) {
	struct ast_entry *a, *b;
	/* Assume conditions are full X=Y form ones for ast purposes for now */

	a = expression();

	if(tok_is(tokn_lt) || tok_is(tokn_gt) ||
	   tok_is(tokn_le) || tok_is(tokn_ge) ||
	   tok_is(tokn_ne) || tok_is(tokn_eq)) {
		emit_noindent("<cond> ");

		ast_emit(tok);

		next_token();

		b = expression();

		ast_append(a);
		ast_append(b);

		ast_close();
	}
	else {
		ast_append(a);
	}
}

int assign(void) {
	struct token *t = tok_get(tok);

	if(accept(tokn_label) && tok_is(tokn_eq)) {
		ast_emit(tok);
		ast_emit_leaf(t);
		tok_put(t);

		next_token();

		ast_append(expression());
		ast_close();

		return 1;
	}

	tok_put(t);

	return 0;
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
		ast_append(expression());
		if(tok_is(tokn_comma))
			emit_noindent(", ");
	} while(accept(tokn_comma));
}

void input_param_list(void) {
	do {
		struct token *t;

		if(accept(tokn_return))
			emit_noindent("RETURN ");

		t = tok_get(tok);
		expect(tokn_label);

		emit_noindent("<label>");

		ast_emit_leaf(t);
		tok_put(t);

		if(tok_is(tokn_comma))
			emit_noindent(", ");

	} while(accept(tokn_comma));
}

void definition(void) {
	struct ast_entry *a;
	struct token *t = tok_get(tok);

	emit_i("DEF ");
	if(accept(tokn_proc)) {
		emit_noindent("PROC");

		ast_emit(t);
		tok_put(t);
		t = tok_get(tok);

		expect(tokn_label);

		a = ast_emit_leaf(t);
		tok_put(t);
		ast_index(a, "proclabel");

		emit_noindent("<label>");

		t = tok_get(tok);

		if(accept(tokn_oparen)) {
			ast_emit(t);

			emit_noindent(" ( ");
			input_param_list();
			expect(tokn_cparen);
			emit_noindent(" ) ");

			ast_close();
		}
		tok_put(t);

		ast_emit_block();
		emit_noindent(" {\n");

		if(accept(tokn_colon)) {
			statement_list(); /* What about IF and ENDPROC? */
			emit_noindent("\n");
		}

		expect(tokn_eol);

		while(!accept(tokn_endproc))
			line();

		emit_o("} ENDPROC\n");
		ast_close();
		ast_close();

	}
	else if(accept(tokn_fn)) {

		emit_noindent("FN");

		ast_emit(t);
		tok_put(t);
		t = tok_get(tok);

		expect(tokn_label);

		a = ast_emit_leaf(t);
		tok_put(t);
		ast_index(a, "fnlabel");

		emit_noindent("<label>");

		t = tok_get(tok);

		if(accept(tokn_oparen)) {
			ast_emit(t);

			emit_noindent(" ( ");
			input_param_list();
			expect(tokn_cparen);
			emit_noindent(" )");

			ast_close();
		}
		tok_put(t);

		ast_emit_block();
		emit_noindent(" {\n");

		if(accept(tokn_colon)) {
			statement_list(); /* What about IF and = ? */
			emit_noindent("\n");
		}

		expect(tokn_eol);

		while (!accept(tokn_eq))
			line(); /* What about IF and = ? */

		emit("RETURN ");

		ast_append(expression());

		emit_noindent("\n");
		emit_o("} ENDFN\n");
		ast_close();
		ast_close();
	}
	else {
		tok_put(t);
		printf("Unknown def type\n");
		exit(1);
	}

	expect(tokn_eol);
}

void statement(void) {
	struct token *t = tok_get(tok);

	if(accept(tokn_if)) {
		emit("IF (");
		ast_emit(t);
		tok_put(t);

		condition();

		emit_noindent(") {\n");
		indent_l++;

		ast_emit_block();
		if(accept(tokn_then) && accept(tokn_eol)) {
			while(!tok_is(tokn_endif)) {

				line();

				if(accept(tokn_else)) {
					ast_close();
					emit_o("}\n");
					emit_i("ELSE {\n");

					ast_emit_block();
					if(tok_is(tokn_if))
						line();
					else
						expect(tokn_eol);

					while (!tok_is(tokn_endif))
						line();
				}

			}

			expect(tokn_endif);
			ast_close();
		}
		else {
			statement_list();

			if (accept(tokn_else)) {
				ast_close();
				emit_noindent("\n");
				emit_o("}\n");
				emit_i("ELSE {\n");
				ast_emit_block();

				statement_list();
			}

			emit_noindent("\n");

			accept(tokn_endif);

			ast_close();
		}
		emit_o("} ENDIF");
		ast_close(); /* Close AST for IF */
	}
	else if(accept(tokn_case)) {
		emit_i("CASE ");

		ast_emit(t);
		tok_put(t);

		ast_append(expression());

		expect(tokn_of);
		emit_noindent("OF {\n");
		expect(tokn_eol);

		while(!tok_is(tokn_endcase)) {
			t = tok_get(tok);
			if(accept(tokn_when)) {
				ast_emit(t);
				emit_i("WHEN ");
				expr_list();
				emit_noindent(" {\n");
			}
			else {
				expect(tokn_otherwise);
				ast_emit(t);
				emit_i("OTHERWISE {\n");
			}

			tok_put(t);

			ast_emit_block();

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
			ast_close();
			emit_o("}\n");
			ast_close();
		}
		expect(tokn_endcase);
		emit_o("} ENDCASE");
		ast_close();
	}
	else if (accept(tokn_proc)) {
		emit("PROC");
		ast_emit(t);
		tok_put(t);
		t = tok_get(tok);
		expect(tokn_label);
		emit_noindent("<label>");
		ast_emit_leaf(t); /* label of PROC to call */
		tok_put(t);
		t = tok_get(tok);
		if(accept(tokn_oparen)) {
			ast_emit(t);
			emit_noindent(" ( ");
			expr_list();
			expect(tokn_cparen);
			ast_close();
			emit_noindent(")");
		}
		tok_put(t);
		ast_close();
	}
	else if (accept(tokn_fn)) {
		emit("FN");
		ast_emit(t);
		tok_put(t);
		t = tok_get(tok);
		expect(tokn_label);
		ast_emit_leaf(t); /* label of FN to call */
		tok_put(t);
		t = tok_get(tok);
		emit_noindent("<label>");
		if(accept(tokn_oparen)) {
			ast_emit(t);
			emit_noindent(" ( ");
			expr_list();
			expect(tokn_cparen);
			emit_noindent(" )");
			ast_close();
		}
		tok_put(t);
		ast_close();
	}
	else if (accept(tokn_repeat)) {
		emit_i("REPEAT {\n");
		ast_emit(t);
		tok_put(t);
		ast_emit_block();

		if(accept(tokn_colon))
			statement_list();
		else
			expect(tokn_eol);

		while(!accept(tokn_until))
			line();

		emit_o("} UNTIL ( ");

		ast_close();

		condition();

		emit_noindent(")");

		ast_close();
	}
	else if (accept(tokn_while)) {
		emit_i("WHILE ( ");

		emit(tokn_while);
		ast_emit(t);
		tok_put(t);

		condition();

		emit_noindent(") {\n");

		ast_emit_block();

		if(accept(tokn_colon))
			statement_list();
		else 
			expect(tokn_eol);

		while(!accept(tokn_endwhile))
			line();

		ast_close();

		emit_o("} ENDWHILE");

		ast_close();
	}
	else if (accept(tokn_goto)) {
		emit("GOTO ");

		ast_emit(t);
		tok_put(t);

		t = tok_get(tok);
		expect(tokn_label);

		ast_emit_leaf(t);
		tok_put(t);

		emit_noindent("<label>");

		ast_close();
	}
	else if(accept(tokn_print)) {
		emit("PRINT");

		ast_emit(t);
		tok_put(t);

		if(!tok_is(tokn_eol) && !tok_is(tokn_colon)) {
			emit_noindent(" {\n");
			indent_l++;
			do {
				indent;
				ast_append(expression());

				emit_noindent("\n");
			} while(accept(tokn_semicolon) && !tok_is(tokn_eol) && !tok_is(tokn_colon));
			emit_o("}");
		}
		ast_close();
	}
	else if (accept(tokn_end)) {
		ast_emit_leaf(t);
		tok_put(t);
		emit("END");
	}
	else {
		tok_put(t);
		assign();
	}
}

void line(void) {
	struct ast_entry *a;

	if(tok_is(tokn_eol))
		goto out; /* Empty line */

	if(tok_is(tokn_label)) {
		struct token *t = tok_get(tok);
		if(!assign()) {
			if(expect(tokn_colon)) {
				emit("<label>:");
				a = ast_emit_leaf(t);
				ast_index(a, "label");
				tok_put(t);
				goto out; /* Labels must be on their own at the start of a line? */
			}
		}
		tok_put(t);
	}
	else if(accept(tokn_colon)) {
		; /* Skip leading : at beginning of lines */
	}
	else if(accept(tokn_library)) {
		emit("LIBRARY ");
		expect(tokn_value);
		emit_noindent("<filename>");
	}
	else if(tok_is(tokn_static) || tok_is(tokn_global) || tok_is(tokn_const)) {
		if(tok_is(tokn_static))
			emit("STATIC ");
		if(tok_is(tokn_global))
			emit("GLOBAL ");
		if(tok_is(tokn_const))
			emit("CONST ");

		next_token();

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

	ast_emit_block();

	next_token();

	while(1) {
		toplevel_line();
	}

	ast_close();

	return 0;
}



int main(void) {

	tokeniser_init();

	ast_new_context(ast_program);

	fd = open("test.bas", O_RDONLY);
	if(fd == -1) {
		printf("Couldnt open file\n");
		exit(1);
	}

	parse(fd);

	return 0;
}
