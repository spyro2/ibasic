#ifndef TYPES_H_INCLUDED

enum type_id {
	type_int, type_float, type_string, type_frame, type_unspec,
	type_a_int,
};

union imm_storage {
	void *v;
	char *s;
	char *name;
	float d;
	int i;
	int *ip; // TO BE DELETED
};

struct imm_value {
	enum type_id type;
	union imm_storage data;
	int ref;
	int size; // To be deleted
	char *name; // To be deleted
};

#define TYPES_H_INCLUDED
#endif

