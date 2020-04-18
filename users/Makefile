CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

user.o: user.c ../ll/ll.h

.PHONY: clean
clean:
	rm -f *.o *~ #*

.PHONY: all
all: clean user.o
