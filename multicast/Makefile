CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = -g -lm

LDLIBS =

multicast.o: multicast.c multicast.h ../trees/trees.h

.PHONY: clean
clean:
	rm -f *.o

.PHONY: all
all: clean multicast.o
