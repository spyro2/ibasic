#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "ast.h"
#include "parse.h"
#include "tokeniser.h"

int main(void) {
	int fd;

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
