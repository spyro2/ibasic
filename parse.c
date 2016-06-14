#include <stdio.h>
#include <stdlib.h>

#include "tokeniser.h"
#include "stack.h"
#include "expression.h"
#include "ast.h"

#include "colours.h"

//#define PRINT_NEXT_TOKEN

static struct token *tok = NULL;
static int fd;

static int tok_is(enum tokid id) {
	return tok->id == id ? 1 : 0;
}

static void next_token(void) {
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

static int accept(enum tokid id) {

	if (tok->id == id) {
		next_token();
		return 1;
	}

	return 0;
}

static int expect(enum tokid id) {
	struct symbol *s;

	if (accept(id))
		return 1;

	s = sym_from_id(tok->id);
	printf("Unexpected token: %d (%s)\n", tok->id, s->name?s->name:"Unknown");
	s = sym_from_id(id);
	printf("Expected: %d (%s)\n", id, s->name?s->name:"Unknown");

	exit(1);

	return 0;
}

static int get_precedence(struct token *a) {

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

static int preceeds(struct token *a, struct token *b) {
	int pa = get_precedence(a), pb = get_precedence(b);

	return pa > pb;
}

static void do_expression(struct stack *output, struct stack *operator);
static struct ast_entry *expression(void);
static void expr_list(void);

static void factor(struct stack *output, struct stack *operator){

	/* Unary operators */
	if(tok_is(tokn_plus) || tok_is(tokn_minus)) {
		tok_get(tok);

		/* Promote token to a unary one */
		if(tok_is(tokn_plus))
			tok->id = tokn_uplus;
		else
			tok->id = tokn_uminus;

		if(!preceeds(tok, peek(operator)))
			push(output, pop(operator));

		push(operator, tok);

		next_token();
	}

	if(tok_is(tokn_plus) || tok_is(tokn_minus)) {
		printf("Syntax error!\n");
		exit(1);
	}

	if(tok_is(tokn_value)) {
		tok_get(tok);
		push(output, tok);

		next_token();
	}
	else if(tok_is(tokn_label)) {
		struct token *t = tok_get(tok);

		next_token();

		if(tok_is(tokn_osquare)) {
			t->id = tokn_array;

			tok_get(tok);
			push(operator, tok);

			next_token();

			do_expression(output, operator);

			expect(tokn_csquare);

			tok_put(pop(operator)); /* pop the open square bracket */
		}

		push(output, t);
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

static void term(struct stack *output, struct stack *operator) {

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

static void do_expression(struct stack *output, struct stack *operator) {
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
	while((t = peek(operator)) &&
	      !(tokid(t) == tokn_oparen || tokid(t) == tokn_osquare))
		push(output, pop_nocheck(operator));

}

static void expr_emit_ast(struct stack *o) {
	struct token *t = pop(o);
	int i = tokid(t);

	if(i == tokn_label || i == tokn_value) {
		ast_emit_leaf(t);
	}
	else if(i == tokn_uplus || i == tokn_uminus || i == tokn_array) {
		ast_emit(t);

		expr_emit_ast(o);

		ast_close();
	}
	else if (i == tokn_plus || i == tokn_minus || i == tokn_asterisk ||
	         i == tokn_slash) {
		ast_emit(t);

		expr_emit_ast(o);
		expr_emit_ast(o);

		ast_close();
	}
	else if(i == tokn_fn) {
		int n = t->val->data.i;
		struct token *tn;

		ast_emit(t);

		/* FN name */
		tn = pop(o);
		ast_emit_leaf(tn);
		tok_put(tn);

		while(n) {
			expr_emit_ast(o);

			n--;
		}


		ast_close();
	}
	else {
		printf("Unknown operator!\n");
		exit(1);
	}

	tok_put(t);
}

static struct ast_entry *expression() {
	struct stack output = {{0}}, operator = {{0}};

	/* Build RPN form of an expression */
	do_expression(&output, &operator);

	if(peek(&output)) {
		struct ast_entry *last = ast_get_context();
		struct ast_entry *a = ast_new_context(ast_expression);

		expr_emit_ast(&output);

		ast_set_context(last);

		return a;
	}

	return NULL;
}

static void condition(void) {
	struct ast_entry *a, *b;
	/* Assume conditions are full X=Y form ones for ast purposes for now */

	a = expression();

	if(tok_is(tokn_lt) || tok_is(tokn_gt) ||
	   tok_is(tokn_le) || tok_is(tokn_ge) ||
	   tok_is(tokn_ne) || tok_is(tokn_eq)) {

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

static int assign(void) {
	struct token *t = tok_get(tok);

	if(accept(tokn_label)) {
		if(accept(tokn_osquare)) {
			struct ast_entry *a;

			a = expression();

			expect(tokn_csquare);

			if(tok_is(tokn_eq)) {
				tok->id = tokn_assign;
				ast_emit(tok);

				t->id = tokn_array;
				ast_emit(t);
				tok_put(t);

				ast_append(a);
				ast_close();

				next_token();

				ast_append(expression());
				ast_close();
			}
			else {
				printf("Arrays cannot be here labels\n");
				exit(1);
			}

			return 1;
		}
		else if(tok_is(tokn_eq)) {
			tok->id = tokn_assign;
			ast_emit(tok);

			ast_emit_leaf(t);
			tok_put(t);

			next_token();

			ast_append(expression());
			ast_close();

			return 1;
		}
	}

	tok_put(t);

	return 0;
}


static void line(void);
static void statement(void);

static void statement_list(void) {
	int d = 0;

	do {
		statement();
	} while((d = accept(tokn_colon)));
}

static void expr_list(void) {
	do {
		ast_append(expression());
	} while(accept(tokn_comma));
}

static void input_param_list(void) {
	struct stack output = {{0}};
	struct token *t;

	do {
		if(accept(tokn_return))
			; // FIXME

		t = tok_get(tok);
		expect(tokn_label);

		push(&output, t);

	} while(accept(tokn_comma));

	while (peek(&output)) {
		t = pop(&output);
		ast_emit_leaf(t);
		tok_put(t);
	}
}

static void definition(void) {
	struct ast_entry *a;
	struct token *t = tok_get(tok);

	if(accept(tokn_proc)) {

		t->id = ast_proc;
		a = ast_emit(t);
		tok_put(t);
		t = tok_get(tok);

		expect(tokn_label);

		ast_emit_leaf(t);
		ast_index(a, t->val->data.s);
		tok_put(t);

		if(accept(tokn_oparen)) {
			input_param_list();
			expect(tokn_cparen);
		}

		ast_emit_block();

		if(accept(tokn_colon))
			statement_list(); /* What about IF and ENDPROC? */

		expect(tokn_eol);

		while(!accept(tokn_endproc))
			line();

		ast_close();
		ast_close();

	}
	else if(accept(tokn_fn)) {

		t->id = ast_fn;
		a = ast_emit(t);
		tok_put(t);
		t = tok_get(tok);

		expect(tokn_label);

		ast_emit_leaf(t);
		ast_index(a, t->val->data.s);
		tok_put(t);

		if(accept(tokn_oparen)) {
			input_param_list();
			expect(tokn_cparen);
		}

		ast_emit_block();

		if(accept(tokn_colon))
			statement_list(); /* What about IF and = ? */

		expect(tokn_eol);

		while (!accept(tokn_eq))
			line(); /* What about IF and = ? */

		ast_append(expression());

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

static void statement(void) {
	struct token *t = tok_get(tok);

	if(accept(tokn_if)) {
		ast_emit(t);
		tok_put(t);

		condition();

		ast_emit_block();
		if(accept(tokn_then) && accept(tokn_eol)) {
			while(!tok_is(tokn_endif)) {

				line();

				if(accept(tokn_else)) {
					ast_close();

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
				ast_emit_block();

				statement_list();
			}

			accept(tokn_endif);

			ast_close();
		}
		ast_close(); /* Close AST for IF */
	}
	else if(accept(tokn_case)) {

		ast_emit(t);
		tok_put(t);

		ast_append(expression());

		expect(tokn_of);
		expect(tokn_eol);

		while(!tok_is(tokn_endcase)) {
			t = tok_get(tok);
			if(accept(tokn_when)) {
				ast_emit(t);
				expr_list();
			}
			else {
				expect(tokn_otherwise);
				ast_emit(t);
			}

			tok_put(t);

			ast_emit_block();

			if(accept(tokn_colon))
				statement_list();

			if(expect(tokn_eol)) {
				while(!(tok_is(tokn_when) ||
				        tok_is(tokn_otherwise) ||
				        tok_is(tokn_endcase))) {
					line();
				}
			}

			ast_close();

			ast_close();
		}
		expect(tokn_endcase);
		ast_close();
	}
	else if (accept(tokn_proc)) {
		ast_emit(t);
		tok_put(t);
		t = tok_get(tok);
		expect(tokn_label);
		ast_emit_leaf(t); /* label of PROC to call */
		tok_put(t);
		t = tok_get(tok);
		if(accept(tokn_oparen)) {
			expr_list();
			expect(tokn_cparen);
		}
		tok_put(t);
		ast_close();
	}
	else if (accept(tokn_fn)) {
		ast_emit(t);
		tok_put(t);
		t = tok_get(tok);
		expect(tokn_label);
		ast_emit_leaf(t); /* label of FN to call */
		tok_put(t);
		t = tok_get(tok);
		if(accept(tokn_oparen)) {
			expr_list();
			expect(tokn_cparen);
		}
		tok_put(t);
		ast_close();
	}
	else if (accept(tokn_repeat)) {
		ast_emit(t);
		tok_put(t);
		ast_emit_block();

		if(accept(tokn_colon))
			statement_list();
		else
			expect(tokn_eol);

		while(!accept(tokn_until))
			line();

		ast_close();

		condition();

		ast_close();
	}
	else if (accept(tokn_while)) {
		ast_emit(t);
		tok_put(t);

		condition();

		ast_emit_block();

		if(accept(tokn_colon))
			statement_list();
		else 
			expect(tokn_eol);

		while(!accept(tokn_endwhile))
			line();

		ast_close();

		ast_close();
	}
	else if (accept(tokn_for)) {
		ast_emit(t);
		tok_put(t);

		if(!assign()) {
			printf("Expected assignment\n");
			exit(1);
		}

		expect(tokn_to);

		ast_append(expression());

		if(accept(tokn_step))
			ast_append(expression());

		ast_emit_block();

		if(accept(tokn_colon))
			statement_list();
		else
			expect(tokn_eol);

		while(!accept(tokn_next))
			line();

		ast_close();

		ast_close();
	}
	else if (accept(tokn_break) || accept(tokn_continue)) {
		ast_emit_leaf(t);
		tok_put(t);
	}
	else if (accept(tokn_goto)) {
		ast_emit(t);
		tok_put(t);

		t = tok_get(tok);
		expect(tokn_label);

		ast_emit_leaf(t);
		tok_put(t);

		ast_close();
	}
	else if(accept(tokn_print)) {
		ast_emit(t);
		tok_put(t);

		if(!tok_is(tokn_eol) && !tok_is(tokn_colon)) {
			do {
				ast_append(expression());

			} while(accept(tokn_semicolon) &&
			        !tok_is(tokn_eol) && !tok_is(tokn_colon));
		}
		ast_close();
	}
	else if (accept(tokn_dim)) {
		ast_emit(t);
		tok_put(t);

		t = tok_get(tok);
		expect(tokn_label);

		t->id = tokn_array;
		ast_emit(t);
		tok_put(t);

		expect(tokn_osquare);

		ast_append(expression());

		expect(tokn_csquare);

		ast_close();

		ast_close();
	}
	else if (accept(tokn_end)) {
		ast_emit_leaf(t);
		tok_put(t);
	}
	else {
		tok_put(t);
		assign();
	}
}

static void line(void) {
	struct ast_entry *a;

	if(tok_is(tokn_eol))
		goto out; /* Empty line */

	if(tok_is(tokn_label)) {
		struct token *t = tok_get(tok);
		if(!assign()) {
			if(expect(tokn_colon)) {
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
		expect(tokn_value);
	}
	else if(tok_is(tokn_static) || tok_is(tokn_global) || tok_is(tokn_const)) {
		if(tok_is(tokn_static))
			;
		if(tok_is(tokn_global))
			;
		if(tok_is(tokn_const))
			;

		next_token();

		expect(tokn_label);

		if(accept(tokn_eq))
			expression();
	}

	statement_list();

out:
	expect(tokn_eol);
}

static void toplevel_line(void) {

	if (accept(tokn_def))
		definition();
	else
		line();

}

int parse (int fd_i) {
	fd = fd_i;

	ast_emit_block();

	next_token();

	do {
		toplevel_line();
	} while (!tok_is(tokn_eof));

	ast_close();

	tok_put(tok);

	printf("Done parsing file\n");

	return 0;
}

