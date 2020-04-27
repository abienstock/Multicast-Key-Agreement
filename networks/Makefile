CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

manager: manager.o ../ll/ll.o

manager.o: manager.c ../ll/ll.h

user: user.o

user.o: user.c

.PHONY: clean
clean:
	rm -f *.o a.out *manager *user *~ #*

.PHONY: all
all: clean manager user
