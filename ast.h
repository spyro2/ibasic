int ast_ind = 0;
#define ast_indent \
	do { \
		int i; \
		for(i = 0 ; i < ast_ind ; i++) \
			printf("\t"); \
	} while(0)

#define ast_emit(a) \
	do { \
		struct symbol *s = sym_from_id((a)); \
		ast_indent;\
		switch((a)) { \
			case ast_block: printf("ast_block( ");break; \
			case ast_proc: printf("ast_proc( ");break; \
			case ast_fn: printf("ast_fn( ");break;	 \
			case tokn_label: printf("cond ( ");break;	 \
			case tokn_value: printf("cond ( ");break;	 \
			default: \
				if(s && s->name)\
					printf("%s( ", s->name); \
				else \
					printf("[%d]( ", (a)); \
		} \
		printf("\n"); \
		ast_ind++; \
	} while(0)

#define ast_emit_leaf(a) \
	do { \
		struct symbol *s = sym_from_id((a)); \
		ast_indent;\
		switch((a)) { \
			case tokn_value: printf("value, ");break;	 \
			case tokn_label: printf("label, ");break;	 \
			case ast_in_param: printf("in_param, ");break;	 \
			default: \
				if(s && s->name)\
					printf("%s, ", s->name); \
				else \
					printf("[%d], ", (a)); \
		} \
		printf("\n"); \
	} while(0)

#define ast_close() ast_ind--;ast_indent;printf(")\n");

#define ast_index(a,b);

