mysh: main.o checkredirect.o addon.o shell.h
	gcc -o mysh main.o addon.o checkredirect.o

addon.o:addon.c
		gcc -c addon.c

checkredirect.o:checkredirect.c
	gcc -c checkredirect.c

main.o:main.c
	gcc -c main.c
