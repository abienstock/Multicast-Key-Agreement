CC = gcc
CXX = g++

INCLUDES = -I../libbotan/include

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS = -L../libbotan/lib -lbotan-2

user.o: user.c user.h ../ll/ll.h ../crypto/crypto.h ../utils.h

.PHONY: clean
clean:
	rm -f *.o *~ *user_driver #*

.PHONY: all
all: clean user.o
