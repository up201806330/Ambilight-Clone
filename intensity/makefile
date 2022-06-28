IFLAGS=-I/usr/X11R6/include -I/usr/local/include -Iinclude
LFLAGS=-L/usr/X11R6/lib -L/usr/local/lib -lrt -pthread

all: ../intensity.app

../intensity.app: main.cpp
	g++ -Wall $< -o $@ $(IFLAGS) $(LFLAGS)
