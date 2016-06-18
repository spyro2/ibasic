#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokid.h"
#include "stack.h"
#include "ast.h"
#include "interpreter.h"
#include "colours.h"
#include "types.h"

#define IS_STRING(a) ((a)->type == type_string)
#define IS_INT(a) ((a)->type == type_int)
#define IS_FLOAT(a) ((a)->type == type_float)
#define IS_NUM(a) (IS_INT(a) || IS_FLOAT(a))

static struct imm_value *do_eval(struct ast_entry *o);

#define call_do_eval(v, e) \
        do { \
        v = do_eval(e); \
        if(!v) \
                v = stack_pop(); \
        } while(0)

void call_proc_or_fn(struct ast_entry *o, struct imm_value *r) {
	struct ast_entry *a, *b;
	struct ast_entry *f = ast_lookup(o);
	struct imm_value *fr;

	if(!f)
		exit(1);

	if(o->children != f->children - 1) {
		printf("Argument count mismatch!\n");
		exit(1);
	}

	b = f->child->next;
	a = o->child->next;

	fr = stack_alloc_frame();

	for (int n = o->children - 1 ; n ; n--) {
		struct imm_value *p = stack_alloc(b->val->data.s);
		struct imm_value *v;

		call_do_eval(v, a);

		if(b->id == tokn_label) {
			if(v->type == type_a_int) {
				printf("Cannot pass array as integer\n");
				exit(1);
			}
			p->type = v->type;
			p->data.i = v->data.i;
		}
		else if(b->id == tokn_array) {
			if(v->type != type_a_int) {
				printf("Cannot pass int as array!\n");
				exit(1);
			}
			p->type = v->type;
			p->data.ip = malloc(v->size * sizeof(int));
			memcpy(p->data.ip, v->data.ip, v->size);
		}

		a = a->next;
		b = b->next;
	}

	stack_set_frame(fr);

	interpret_block(b, r);

	stack_unwind_frame();
}

static struct imm_value *do_eval(struct ast_entry *o) {
	struct imm_value *a, *b, *r = NULL; // FIXME: for testing

	if(!o) {
		printf("NULL element in expression\n");
		exit(1);
	}

	if(o->id == ast_expression)
		o = o->child;

	switch (o->id) {
		case tokn_label:
			a = stack_lookup_var(o->val->data.s);

			if(!a) {
				printf("No such variable! (%s)\n", o->val->data.s);
				exit(1);
			}

			return a;

		case tokn_value:
			return o->val;

		case tokn_array:
			r = stack_alloc(NULL);

			a = stack_lookup_var(o->val->data.s);

			if(!a) {
				printf("No such array! (%s)\n", o->val->data.s);
				exit(1);
			}

			call_do_eval(b, o->child);

			r->type = type_int; //HACK
			r->data.i = a->data.ip[b->data.i];

			return NULL;

		case tokn_uplus:
			r = stack_alloc(NULL);

			call_do_eval(a, o->child);

			if(!IS_NUM(a)) {
				printf("Unary + requires a number\n");
				exit(1);
			}

			r->type = a->type;

			if(a->type == type_int)
				r->data.i = a->data.i;
			else if(a->type == type_float)
				r->data.d = a->data.d;

			return NULL;

		case tokn_uminus:
			r = stack_alloc(NULL);

			call_do_eval(a, o->child);

			if(!IS_NUM(a)) {
				printf("Unary + requires a number\n");
				exit(1);
			}

			r->type = a->type;

			if(a->type == type_int)
				r->data.i = -a->data.i;
			else if(a->type == type_float)
				r->data.d = -a->data.d;
		
			return NULL;

		case tokn_plus:
			r = stack_alloc(NULL);

			call_do_eval(a, o->child);
			call_do_eval(b, o->child->next);

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

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
			r = stack_alloc(NULL);

			call_do_eval(a, o->child);
			call_do_eval(b, o->child->next);

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

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
			r = stack_alloc(NULL);

			call_do_eval(a, o->child);
			call_do_eval(b, o->child->next);

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

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
			r = stack_alloc(NULL);

			call_do_eval(a, o->child);
			call_do_eval(b, o->child->next);

			if(!IS_NUM(a) || !IS_NUM(b)) {
				printf("Wrong type!\n");
				exit(1);
			}

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
			r = stack_alloc(NULL); // return value
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

struct imm_value *eval(struct ast_entry *p) {
	return do_eval(p->child);
}
