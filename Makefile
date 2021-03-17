all: lib_linkedProcesses.o lib_shellCommands.o lib_linkedShell.a smallsh

lib_linkedProcesses.o: lib_linkedProcesses.c lib_linkedProcesses.h
	gcc -c lib_linkedProcesses.c -o lib_linkedProcesses.o

lib_shellCommands.o: lib_shellCommands.c lib_shellCommands.h
	gcc -c lib_shellCommands.c -o lib_shellCommands.o

lib_linkedShell.a: lib_linkedProcesses.o lib_shellCommands.o
	ar -r lib_linkedShell.a lib_linkedProcesses.o lib_shellCommands.o

smallsh: smallsh.c lib_linkedShell.a
	gcc -g smallsh.c lib_linkedShell.a -o smallsh