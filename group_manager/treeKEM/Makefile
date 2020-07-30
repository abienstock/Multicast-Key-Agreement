CC = gcc
CXX = g++

INCLUDES = 

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS = 

treeKEM.o: treeKEM.c treeKEM.h ../trees/trees.h ../../skeleton.h ../../crypto/crypto.h ../../utils.h

.PHONY: clean
clean:
	rm -f *.o *~ #*

.PHONY: all
all: clean treeKEM.o
