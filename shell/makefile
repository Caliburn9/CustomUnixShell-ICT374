# makefile for echo

myshell: myshell.o token.o command.o shell_builtins.o shell_utils.o input.o
	gcc myshell.o token.o command.o shell_builtins.o shell_utils.o input.o -o myshell

myshell.o: myshell.c token.h command.h shell_builtins.h shell_utils.h input.h
	gcc -Wall -c myshell.c

token.o: token.c token.h
	gcc -Wall -c token.c

command.o: command.c command.h
	gcc -Wall -c command.c

shell_builtins.o: shell_builtins.c shell_builtins.h
	gcc -Wall -c shell_builtins.c

shell_utils.o: shell_utils.c shell_utils.h
	gcc -Wall -c shell_utils.c

input.o: input.c input.h
	gcc -Wall -c input.c

clean:
	rm -f *.o
