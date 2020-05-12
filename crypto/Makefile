CC = gcc
CXX = g++

INCLUDES = -I../lib/include

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS = -L../lib/lib -lbotan-2

insecure.o: insecure.c crypto.h

secure.o: secure.c crypto.h

.PHONY: clean
clean:
	rm -f *.o a.out *secure *~ #*

.PHONY: all
all: clean secure.o insecure.o
