#ifndef TOKENISER_H_INCLUDED

#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "tokid.h"

struct token {
	enum tokid id;
	struct imm_value *val;
        struct token *next;
	int ref;
};

int tokeniser_init (void);
void tokeniser_exit(void);
struct token *get_next_token(int fd);

char *sym_from_id(enum tokid id);

static inline struct imm_value *val_get(struct imm_value *v) {
	v->ref++;

	return v;
}

static inline void val_put(struct imm_value *v) {
	v->ref--;

	assert(v->ref >= 0);

	// This is not a bug. At present, immediate values emitted by the
	// tokeniser, which have string values, are allocated in one malloc
	// so freeing the value also frees the data.

	if(v->ref == 0)
		free(v);
}

static inline struct token *tok_get(struct token *t) {
	t->ref++;

	if(t->val)
		val_get(t->val);

	return t;
}

static inline void tok_put(struct token *t) {
	t->ref--;

	assert(t->ref >= 0);

	val_put(t->val);

	if(t->ref == 0)
		free(t);
}

#define TOKENISER_H_INCLUDED
#endif

