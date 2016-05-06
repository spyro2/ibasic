#include <stdio.h>
#include <tokeniser.h>


int parse (void) {
	while(t = get_next_token()) {
		printf("next token: %s\n", t->name);
	}

	return 0;
}



int main(void) {

	parse();

	return 0;
}
