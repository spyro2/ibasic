
all:
	gcc -g -Wall -c tokeniser.c
	gcc -g -Wall -c stack.c
	gcc -g -Wall -c expression.c
	gcc -g -Wall parse.c tokeniser.o stack.o expression.o -o ibasic

clean:
	rm *.o ibasic
