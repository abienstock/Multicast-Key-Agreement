CC = gcc
CXX = g++

INCLUDES = 

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS = 

user.o: user.c user.h ../ll/ll.h ../crypto/crypto.h ../utils.h

.PHONY: clean
clean:
	rm -f *.o *~ user.o

.PHONY: all
all: clean user.o
