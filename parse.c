#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tokeniser.h"


int parse (int fd) {
	struct line_entry *le;

	do {
		le = get_next_le(fd, NULL);
		if(le) {
			tok_print_one(le);
	}

	} while(le);

	return 0;
}



int main(void) {
	int fd;

	tokeniser_init();

	fd = open("test.bas", O_RDONLY);
	if(fd == -1) {
		printf("Couldnt open file\n");
		exit(1);
	}

	parse(fd);

	return 0;
}
