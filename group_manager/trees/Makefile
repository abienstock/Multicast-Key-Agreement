CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

LBBT-test: LBBT-test.o LBBT.o ../../ll/ll.o tree-utils.o

LBBT-test.o: LBBT-test.c trees.h LBBT.o ../../ll/ll.h

LBBT.o: LBBT.c trees.h ../../ll/ll.h

tree-utils.o: tree-utils.c trees.h

ll.o: ../../ll/ll.c ../../ll/ll.h


.PHONY: clean
clean:
	rm -f *.o *~ LBBT-test #*

.PHONY: all
all: clean LBBT-test
