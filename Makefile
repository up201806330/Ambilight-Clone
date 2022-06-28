IDIR=include
SDIR=src
ODIR=obj

IFLAGS=-I/usr/X11R6/include -I/usr/local/include -Iinclude
LFLAGS=-L/usr/X11R6/lib -L/usr/local/lib -lX11 -lXext -lrt -pthread

all: screenreader

OFILES=\
	$(ODIR)/ScreenReader.o \
	$(ODIR)/ScreenProcessor.o \
	$(ODIR)/LedProcessor.o

screenreader: $(SDIR)/main.cpp $(OFILES)
	g++ -Wall $< $(OFILES) -o $@ $(IFLAGS) $(LFLAGS)

obj/%.o: $(SDIR)/%.cpp | $(ODIR)
	g++ -Wall -c $< -o $@ $(IFLAGS) $(LFLAGS)

$(ODIR):
	mkdir -p $@
