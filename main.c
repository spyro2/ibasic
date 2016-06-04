#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "ast.h"
#include "parse.h"
#include "tokeniser.h"

int main(int argc, char *argv[]) {
	int fd;

	if (argc < 2) {
		fprintf(stderr, "No input file!\n");
		exit(1);
	}

	fd = open(argv[1], O_RDONLY);
	if(fd == -1) {
		printf("Couldnt open file %s\n", argv[1]);
		exit(1);
	}

	tokeniser_init();

	ast_new_context(ast_program);

	parse(fd);

	tokeniser_exit();

	return 0;
}
