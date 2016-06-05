#include <stdio.h>
#include <stdlib.h>

#include "tokeniser.h"
#include "stack.h"
#include "ast.h"
#include "interpretter.h"
#include "colours.h"

struct value *val_alloc(void) {
	struct value *v = calloc(1, sizeof(*v));

	if(!v)
		exit(1);

	v->flags = VAL_ALLOC;

	return v;
}

#define IS_STRING(a) ((a)->type == type_string)
#define IS_INT(a) ((a)->type == type_int)
#define IS_FLOAT(a) ((a)->type == type_float)
#define IS_NUM(a) (IS_INT(a) || IS_FLOAT(a))

void do_basic_eval(struct stack *o) {
	struct token *t = pop(o);
	int i = tokid(t);

	if(i == tokn_label || i == tokn_value) {
		ast_emit_leaf(t);
	}
	else if(i == tokn_uplus || i == tokn_uminus) {
		ast_emit(t);

		do_basic_eval(o);

		ast_close();
	}
	else if (i == tokn_plus || i == tokn_minus || i == tokn_asterisk ||
	         i == tokn_slash) {
		ast_emit(t);

		do_basic_eval(o);
		do_basic_eval(o);

		ast_close();
	}
	else if(i == tokn_fn) {
		int n = t->val->data.i;
		struct token *tn;

		ast_emit(t);

		/* FN name */
		tn = pop(o);

		while(n) {
			do_basic_eval(o);

			n--;
		}

		ast_emit_leaf(tn);
		tok_put(tn);

		ast_close();
	}
	else {
		printf("Unknown operator!\n");
		exit(1);
	}

	tok_put(t);
}

struct ast_entry *basic_eval(struct stack *o) {
	if(peek(o)) {
		struct ast_entry *last = ast_get_context();
		struct ast_entry *a = ast_new_context(ast_expression);

		do_basic_eval(o);

		ast_set_context(last);

		return a;
	}

	return NULL;
}

struct value *eval(struct ast_entry *o) {
	struct value *a, *b, *c;

	if(!o) {
		printf("NULL element in expression\n");
		exit(1);
	}

	switch (o->id) {
		case tokn_label:
			a = lookup_var(o->val->data.s);
			val_get(a);

			return a;

		case tokn_value:
			val_get(o->val);

			return o->val;

		case tokn_uplus:
			a = eval(o->child);

			if(!IS_NUM(a)) {
				printf("Unary + requires a number\n");
				exit(1);
			}

			val_get(a);

			return a;

		case tokn_uminus:
			a = eval(o->child);

			if(!IS_NUM(a)) {
				printf("Unary + requires a number\n");
				exit(1);
			}

			b = val_alloc();
			b->type = a->type;
			b->flags |= VAL_READONLY;

			if(a->type == type_int)
				b->data.i = -a->data.i;
			else if(a->type == type_float)
				b->data.d = -b->data.d;
		
			val_get(b);

			val_put(a);

			return b;

		case tokn_plus:
			a = eval(o->child); b = eval(o->child->next);

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

			c = val_alloc();
			c->flags |= VAL_READONLY;
			if(IS_FLOAT(a) || IS_FLOAT(b))
				c->type = type_float;

			if(IS_FLOAT(b)) {
				if(IS_FLOAT(c))
					c->data.d = b->data.d;
				else
					c->data.i = (int)b->data.d;
			}
			else {
				if(IS_FLOAT(c))
					c->data.d = (double)b->data.i;
				else
					c->data.i = b->data.i;
			}
			if(IS_FLOAT(a)) {
				if(IS_FLOAT(c))
					c->data.d += a->data.d;
				else
					c->data.i += (int)a->data.d;
			}
			else {
				if(IS_FLOAT(c))
					c->data.d += (double)a->data.i;
				else
					c->data.i += a->data.i;
			}

			val_get(c);
			val_put(a);
			val_put(b);

			return c;

		case tokn_minus:
			a = eval(o->child); b = eval(o->child->next);

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

			c = val_alloc();
			c->flags |= VAL_READONLY;
			if(IS_FLOAT(a) || IS_FLOAT(b))
				c->type = type_float;

			if(IS_FLOAT(b)) {
				if(IS_FLOAT(c))
					c->data.d = b->data.d;
				else
					c->data.i = (int)b->data.d;
			}
			else {
				if(IS_FLOAT(c))
					c->data.d = (double)b->data.i;
				else
					c->data.i = b->data.i;
			}
			if(IS_FLOAT(a)) {
				if(IS_FLOAT(c))
					c->data.d -= a->data.d;
				else
					c->data.i -= (int)a->data.d;
			}
			else {
				if(IS_FLOAT(c))
					c->data.d -= (double)a->data.i;
				else
					c->data.i -= a->data.i;
			}

			val_get(c);

			val_put(a);
			val_put(b);

			return c;

		case tokn_asterisk:
			a = eval(o->child); b = eval(o->child->next);

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

			c = val_alloc();
			c->flags |= VAL_READONLY;
			if(IS_FLOAT(a) || IS_FLOAT(b))
				c->type = type_float;

			if(IS_FLOAT(b)) {
				if(IS_FLOAT(c))
					c->data.d = b->data.d;
				else
					c->data.i = (int)b->data.d;
			}
			else {
				if(IS_FLOAT(c))
					c->data.d = (double)b->data.i;
				else
					c->data.i = b->data.i;
			}
			if(IS_FLOAT(a)) {
				if(IS_FLOAT(c))
					c->data.d *= a->data.d;
				else
					c->data.i *= (int)a->data.d;
			}
			else {
				if(IS_FLOAT(c))
					c->data.d *= (double)a->data.i;
				else
					c->data.i *= a->data.i;
			}

			val_get(c);

			val_put(a);
			val_put(b);

			return c;

		case tokn_slash:
			a = eval(o->child); b = eval(o->child->next);

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

			c = val_alloc();
			c->flags |= VAL_READONLY;
			if(IS_FLOAT(a) || IS_FLOAT(b))
				c->type = type_float;

			if(IS_FLOAT(b)) {
				if(IS_FLOAT(c))
					c->data.d = b->data.d;
				else
					c->data.i = (int)b->data.d;
			}
			else {
				if(IS_FLOAT(c))
					c->data.d = (double)b->data.i;
				else
					c->data.i = b->data.i;
			}
			if(IS_FLOAT(a)) {
				if(IS_FLOAT(c))
					c->data.d /= a->data.d;
				else
					c->data.i /= (int)a->data.d;
			}
			else {
				if(IS_FLOAT(c))
					c->data.d /= (double)a->data.i;
				else
					c->data.i /= a->data.i;
			}

			val_get(c);
			val_put(a);
			val_put(b);

			return c;

		case tokn_fn:
			{
			struct ast_entry *a = o->child;
			int n = o->children - 1;

			while(n) {
				struct value *v = eval(a);
				val_get(v);
				val_push(v);
				a = a->next;
				n--;
			}

			a = ast_lookup(a->val->data.s);
			if(!a) {
				printf("Could not find function\n");
				exit(1);
			}

			val_get(b);

			return b;
			}

		default:

		printf("Error: unknown operator\n");
		exit(1);
	}
}

