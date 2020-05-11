CC = gcc
CXX = g++

INCLUDES = -I../../driver/include

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS = -L../../driver/lib -lbotan-2

multicast.o: multicast.c multicast.h ../trees/trees.h ../../skeleton.h ../../crypto/crypto.h

.PHONY: clean
clean:
	rm -f *.o *~ #*

.PHONY: all
all: clean multicast.o
