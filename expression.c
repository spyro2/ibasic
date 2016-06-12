#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokeniser.h"
#include "stack.h"
#include "ast.h"
#include "interpreter.h"
#include "colours.h"

#define IS_STRING(a) ((a)->type == type_string)
#define IS_INT(a) ((a)->type == type_int)
#define IS_FLOAT(a) ((a)->type == type_float)
#define IS_NUM(a) (IS_INT(a) || IS_FLOAT(a))

static struct value *do_eval(struct ast_entry *o);

void call_proc_or_fn(struct ast_entry *o, struct value *r) {
	struct ast_entry *a = o->child;
	struct ast_entry *f = ast_lookup(a->val->data.s);
	struct ast_entry *b;
	struct value *fr;

	if(!f) {
		printf("Could not find %s%s)\n", o->id == tokn_fn?"function (FN":"procedure (PROC", a->val->data.s);
		exit(1);
	}

	if(o->children != f->children - 1) {
		printf("Argument count mismatch!\n");
		exit(1);
	}

	b = f->child;

	a = a->next;
	b = b->next;

	fr = val_alloc_frame();

	for (int n = o->children - 1 ; n ; n--) {
		struct value *p = val_alloc(b->val->data.s);
		struct value *v;

		v = do_eval(a);
		if(!v)
			v = val_pop();

		p->type = v->type;
		p->data.i = v->data.i;

		a = a->next;
		b = b->next;
	}

	val_set_frame(fr);

	interpret_block(b, r);

	unwind_frame();
}

static struct value *do_eval(struct ast_entry *o) {
	struct value *a, *b, *r = NULL; // FIXME: for testing

	if(!o) {
		printf("NULL element in expression\n");
		exit(1);
	}

	if(o->id == ast_expression)
		o = o->child;

	switch (o->id) {
		case tokn_label:
			a = lookup_var(o->val->data.s);

			if(!a) {
				printf("No such variable! (%s)\n", o->val->data.s);
				exit(1);
			}

			return a;

		case tokn_value:
			return o->val;

		case tokn_uplus:
			r = val_alloc(NULL);

			a = do_eval(o->child);

			if(!a)
				a = val_pop();

			if(!IS_NUM(a)) {
				printf("Unary + requires a number\n");
				exit(1);
			}

			r->type = a->type;
			r->flags |= VAL_READONLY;

			if(a->type == type_int)
				r->data.i = a->data.i;
			else if(a->type == type_float)
				r->data.d = a->data.d;

			return NULL;

		case tokn_uminus:
			r = val_alloc(NULL);

			a = do_eval(o->child);
			if(!a)
				a = val_pop();

			if(!IS_NUM(a)) {
				printf("Unary + requires a number\n");
				exit(1);
			}

			r->type = a->type;
			r->flags |= VAL_READONLY;

			if(a->type == type_int)
				r->data.i = -a->data.i;
			else if(a->type == type_float)
				r->data.d = -a->data.d;
		
			return NULL;

		case tokn_plus:
			r = val_alloc(NULL);

			a = do_eval(o->child);
			if(!a)
				a = val_pop();
			b = do_eval(o->child->next);
			if(!b)
				b = val_pop();

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}


			r->flags |= VAL_READONLY;
			if(IS_FLOAT(a) || IS_FLOAT(b))
				r->type = type_float;
			else
				r->type = type_int;

			if(IS_FLOAT(b)) {
				if(IS_FLOAT(r))
					r->data.d = b->data.d;
				else
					r->data.i = (int)b->data.d;
			}
			else {
				if(IS_FLOAT(r))
					r->data.d = (double)b->data.i;
				else
					r->data.i = b->data.i;
			}
			if(IS_FLOAT(a)) {
				if(IS_FLOAT(r))
					r->data.d += a->data.d;
				else
					r->data.i += (int)a->data.d;
			}
			else {
				if(IS_FLOAT(r))
					r->data.d += (double)a->data.i;
				else
					r->data.i += a->data.i;
			}

			return NULL;

		case tokn_minus:
			r = val_alloc(NULL);

			a = do_eval(o->child);
			if(!a)
				a = val_pop();
			b = do_eval(o->child->next);
			if(!b)
				b = val_pop();

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

			r->flags |= VAL_READONLY;
			if(IS_FLOAT(a) || IS_FLOAT(b))
				r->type = type_float;
			else
				r->type = type_int;

			if(IS_FLOAT(b)) {
				if(IS_FLOAT(r))
					r->data.d = b->data.d;
				else
					r->data.i = (int)b->data.d;
			}
			else {
				if(IS_FLOAT(r))
					r->data.d = (double)b->data.i;
				else
					r->data.i = b->data.i;
			}
			if(IS_FLOAT(a)) {
				if(IS_FLOAT(r))
					r->data.d -= a->data.d;
				else
					r->data.i -= (int)a->data.d;
			}
			else {
				if(IS_FLOAT(r))
					r->data.d -= (double)a->data.i;
				else
					r->data.i -= a->data.i;
			}

			return NULL;

		case tokn_asterisk:
			r = val_alloc(NULL);

			a = do_eval(o->child);
			if(!a)
				a = val_pop();
			b = do_eval(o->child->next);
			if(!b)
				b = val_pop();

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

			r->flags |= VAL_READONLY;
			if(IS_FLOAT(a) || IS_FLOAT(b))
				r->type = type_float;
			else
				r->type = type_int;

			if(IS_FLOAT(b)) {
				if(IS_FLOAT(r))
					r->data.d = b->data.d;
				else
					r->data.i = (int)b->data.d;
			}
			else {
				if(IS_FLOAT(r))
					r->data.d = (double)b->data.i;
				else
					r->data.i = b->data.i;
			}
			if(IS_FLOAT(a)) {
				if(IS_FLOAT(r))
					r->data.d *= a->data.d;
				else
					r->data.i *= (int)a->data.d;
			}
			else {
				if(IS_FLOAT(r))
					r->data.d *= (double)a->data.i;
				else
					r->data.i *= a->data.i;
			}

			return NULL;

		case tokn_slash:
			r = val_alloc(NULL);

			a = do_eval(o->child);
			if(!a)
				a = val_pop();
			b = do_eval(o->child->next);
			if(!b)
				b = val_pop();

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

			r->flags |= VAL_READONLY;
			if(IS_FLOAT(a) || IS_FLOAT(b))
				r->type = type_float;
			else
				r->type = type_int;

			if(IS_FLOAT(b)) {
				if(IS_FLOAT(r))
					r->data.d = b->data.d;
				else
					r->data.i = (int)b->data.d;
			}
			else {
				if(IS_FLOAT(r))
					r->data.d = (double)b->data.i;
				else
					r->data.i = b->data.i;
			}
			if(IS_FLOAT(a)) {
				if(IS_FLOAT(r))
					r->data.d /= a->data.d;
				else
					r->data.i /= (int)a->data.d;
			}
			else {
				if(IS_FLOAT(r))
					r->data.d /= (double)a->data.i;
				else
					r->data.i /= a->data.i;
			}

			return NULL;

		case tokn_fn:
			r = val_alloc(NULL); // return value
			/* Note: be careful, as we have not initialised the
			 * stack entry allocated.
			 */
			call_proc_or_fn(o, r);
			return NULL;
		default:

		printf("Error: unknown operator\n");
		exit(1);
	}
}

struct value *eval(struct ast_entry *p) {
	return do_eval(p->child);
}
