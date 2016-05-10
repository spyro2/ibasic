
all:
	gcc -Wall -c tokeniser.c
	gcc -Wall parse.c tokeniser.o -o ibasic
