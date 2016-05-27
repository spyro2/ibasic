
all:
	gcc -g -Wall -c tokeniser.c
	gcc -g -Wall -c stack.c
	gcc -g -Wall -c expression.c
	gcc -g -Wall -c ast.c
	gcc -g -Wall parse.c tokeniser.o stack.o expression.o ast.o -o ibasic

clean:
	rm *.o ibasic
