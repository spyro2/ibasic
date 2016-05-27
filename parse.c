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
struct token *tok;

void print_current_token(char *s) {
	printf("%s", s);
        tok_print_one(tok);
        printf(ANSI_RESET);
}

int tok_is(enum tokid id) {
	return tok->sym->id == id ? 1 : 0;
}

void next_token(void) {
	/* Skip comments - This may have implications for writing a
	 * pretty-printer.
	 */
	do {
		tok = get_next_token(fd);
	} while (tok_is(tokn_comment));

#if PRINT_NEXT_TOKEN
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
void expression(void);
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
		push(output, tok);

		next_token();
	}
	else if(tok_is(tokn_oparen)) {
		push(operator, tok);

		next_token();

		do_expression(output, operator);

		expect(tokn_cparen);

		pop(operator); /* pop the open parentesis */
	}
	else if(tok_is(tokn_fn)) {
		struct token *t = tok;
		int n_params = 0;

		next_token();

		expect(tokn_label);

		if(tok_is(tokn_oparen)) {

			push(operator, tok);

			next_token();

			do {
				do_expression(output, operator);
				n_params++;
			} while(accept(tokn_comma));

			expect(tokn_cparen);

			pop(operator); /* pop the open parentesis */
		}

		t->val.data.i = n_params;

		push(output, t);
	}
}

void term(struct stack *output, struct stack *operator) {

	factor(output, operator);

	while(tok_is(tokn_asterisk) || tok_is(tokn_slash)) {

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

void expression() {
	struct stack output = {{0}}, operator = {{0}};

	/* Build RPN form of an expression */
	do_expression(&output, &operator);

	/* Display evaluated expression */
#ifdef PRETTYPRINT
	print_expression(&output);
#endif
}

void condition(void) {
	/* Assume conditions are full X=Y form ones for ast purposes for now */

	ast_emit(tok->sym->id);

	expression();
	ast_emit_leaf(tokn_value);

	if(tok_is(tokn_lt) || tok_is(tokn_gt) ||
	   tok_is(tokn_le) || tok_is(tokn_ge) ||
	   tok_is(tokn_ne) || tok_is(tokn_eq)) {
		emit_noindent("<cond> ");
		next_token();
		expression();
		ast_emit_leaf(tokn_value);
	}

	ast_close();
}

void assign(void) {
	expect(tokn_eq); /* FIXME: assignment operator*/
	ast_emit(tokn_eq);
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
		ast_emit_leaf(tokn_value);
		if(tok_is(tokn_comma))
			emit_noindent(", ");
	} while(accept(tokn_comma));
}

int input_param_list(void) {
	int p = 0;

	do {
		if(accept(tokn_return))
			emit_noindent("RETURN ");

		expect(tokn_label);

		emit_noindent("<label>");

		ast_emit_leaf(ast_in_param);

		if(tok_is(tokn_comma))
			emit_noindent(", ");

		p++;
	} while(accept(tokn_comma));

	return p;
}

void definition(void) {
//	struct ast_entry *a;
//	int i;

	emit_i("DEF ");
	if(accept(tokn_proc)) {
		emit_noindent("PROC");

		expect(tokn_label);

		/*a = */ast_emit(ast_proc);
		ast_index(a, "proclabel");

		emit_noindent("<label>");

		if(accept(tokn_oparen)) {
			emit_noindent(" ( ");
			/*i = */input_param_list();
			expect(tokn_cparen);
			emit_noindent(" ) ");
		}
//		a->data = i;

		ast_emit(ast_block);
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

		expect(tokn_label);

		/*a = */ast_emit(ast_fn);
		ast_index(a, "fnlabel");

		emit_noindent("<label>");

		if(accept(tokn_oparen)) {
			emit_noindent(" ( ");
			/*i = */input_param_list();
			expect(tokn_cparen);
			emit_noindent(" )");
		}
//		a->data = i;

		ast_emit(ast_block);
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
		ast_emit_leaf(tokn_value);

		emit_noindent("\n");
		emit_o("} ENDFN\n");
		ast_close();
		ast_close();
	}

	expect(tokn_eol);
}

void statement(void) {
	if(accept(tokn_if)) {
		emit("IF (");
		ast_emit(tokn_if);

		condition();

		emit_noindent(") {\n");
		indent_l++;

		ast_emit(ast_block);
		if(accept(tokn_then) && accept(tokn_eol)) {
			while(!tok_is(tokn_endif)) {

				line();

				if(accept(tokn_else)) {
					ast_close();
					emit_o("}\n");
					emit_i("ELSE {\n");

					ast_emit(ast_block);
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
				ast_emit(ast_block);

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

		ast_emit(tokn_case);

		expression();
		ast_emit_leaf(tokn_value);
		expect(tokn_of);
		emit_noindent("OF {\n");
		expect(tokn_eol);

		while(!tok_is(tokn_endcase)) {
			if(accept(tokn_when)) {
				ast_emit(tokn_when);
				emit_i("WHEN ");
				expr_list();
				emit_noindent(" {\n");
			}
			else {
				expect(tokn_otherwise);
				emit_i("OTHERWISE {\n");
			}

			ast_emit(ast_block);

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
		ast_emit(tokn_proc);
		expect(tokn_label);
		emit_noindent("<label>");
		ast_emit_leaf(-1); /* label of PROC to call */
		if(accept(tokn_oparen)) {
			emit_noindent(" ( ");
			expr_list();
			expect(tokn_cparen);
			emit_noindent(")");
		}
		ast_close();
	}
	else if (accept(tokn_fn)) {
		emit("FN");
		ast_emit(tokn_fn);
		expect(tokn_label);
		ast_emit_leaf(-1); /* label of FN to call */
		emit_noindent("<label>");
		if(accept(tokn_oparen)) {
			emit_noindent(" ( ");
			expr_list();
			expect(tokn_cparen);
			emit_noindent(" )");
		}
		ast_close();
	}
	else if (accept(tokn_repeat)) {
		emit_i("REPEAT {\n");
		ast_emit(tokn_repeat);
		ast_emit(ast_block);

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
		ast_emit(tokn_while);

		condition();

		emit_noindent(") {\n");

		ast_emit(ast_block);

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

		ast_emit(tokn_goto);

		expect(tokn_label);

		ast_emit_leaf(tokn_label);

		emit_noindent("<label>");

		ast_close();
	}
	else if(accept(tokn_print)) {
		emit("PRINT");

		ast_emit(tokn_print);

		if(!tok_is(tokn_eol) && !tok_is(tokn_colon)) {
			emit_noindent(" {\n");
			indent_l++;
			do {
				indent;
				expression();
				ast_emit_leaf(tokn_value);

				emit_noindent("\n");
			} while(accept(tokn_semicolon) && !tok_is(tokn_eol) && !tok_is(tokn_colon));
			emit_o("}");
		}
		ast_close();
	}
	else if(accept(tokn_label)) {
		emit("l-value = ");
		assign();
		ast_emit_leaf(tokn_label);
		expression();
		ast_emit_leaf(tokn_value);
		ast_close();
	}
	else if (accept(tokn_end)) {
		ast_emit_leaf(tokn_end);
		emit("END");
	}
}

void line(void) {
//	struct ast_entry *a;

	if(tok_is(tokn_eol))
		goto out; /* Empty line */

	if(accept(tokn_label)) {
		if(accept(tokn_colon)) {
			emit("<label>:");
			/*a = */ast_emit_leaf(tokn_label);
			ast_index(a, "label");
			goto out; /* Labels must be on their own on a line? */
		}
		else {
			emit("l-value = ");
			assign();
			ast_emit_leaf(tokn_label);
			expression();
			ast_emit_leaf(tokn_value);
			ast_close();
		}
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

	ast_emit(ast_block);

	next_token();
	while(1) {
		toplevel_line();
	}
	ast_close();

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
