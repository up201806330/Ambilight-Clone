all: screenreader

screenreader: screenreader.c
	gcc screenreader.c -o screenreader -I/usr/X11R6/include -I/usr/local/include -L/usr/X11R6/lib -L/usr/local/lib -lX11 -lXext -lrt
