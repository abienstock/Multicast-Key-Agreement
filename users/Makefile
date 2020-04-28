CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

user_driver: user_driver.o user.o ../ll/ll.o ../group_manager/trees/tree-utils.o

user_driver.o: user_driver.c user.h ../skeleton.h ../group_manager/trees/trees.h

user.o: user.c user.h ../ll/ll.h

.PHONY: clean
clean:
	rm -f *.o *~ *user_driver #*

.PHONY: all
all: clean user_driver
