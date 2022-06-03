all: test.o

test.o: test.c
	gcc test.c -o test.o -std=c99 -I/usr/X11R6/include -I/usr/local/include -L/usr/X11R6/lib -L/usr/local/lib -lX11 -lXext
