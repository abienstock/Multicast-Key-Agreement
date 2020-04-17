CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

multicast.o: multicast.c multicast.h ../trees/trees.h ../../skeleton.h

.PHONY: clean
clean:
	rm -f *.o *~ #*

.PHONY: all
all: clean multicast.o
