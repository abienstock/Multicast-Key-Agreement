CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

LBBT-test: LBBT-test.o LBBT.o ../../ll/ll.o tree-utils.o ../../utils.o

LBBT-test.o: LBBT-test.c trees.h LBBT.o ../../ll/ll.h ../../utils.h

LBBT.o: LBBT.c trees.h ../../ll/ll.h ../../skeleton.h ../../utils.h

tree-utils.o: tree-utils.c trees.h ../../skeleton.h


.PHONY: clean
clean:
	rm -f *.o *~ LBBT-test #*

.PHONY: all
all: clean LBBT-test
