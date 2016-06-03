
all:
	gcc -g -Wall -c tokeniser.c
	gcc -g -Wall -c stack.c
	gcc -g -Wall -c expression.c
	gcc -g -Wall -c ast.c
	gcc -g -Wall -c parse.c
	gcc -g -Wall main.c tokeniser.o stack.o expression.o ast.o parse.o -o ibasic

clean:
	rm *.o ibasic
