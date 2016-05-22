
all:
	gcc -g -Wall -c tokeniser.c
	gcc -g -Wall parse.c tokeniser.o -o ibasic
