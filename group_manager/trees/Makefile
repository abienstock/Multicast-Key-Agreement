CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

BTree-test: BTree-test.o BTree.o ../../ll/ll.o tree-utils.o ../../utils.o

BTree.o: BTree.c trees.h ../../ll/ll.h ../../skeleton.h ../../utils.h

LBBT-test: LBBT-test.o LBBT.o ../../ll/ll.o tree-utils.o ../../utils.o

LBBT-test.o: LBBT-test.c trees.h LBBT.o ../../ll/ll.h ../../utils.h

LBBT.o: LBBT.c trees.h ../../ll/ll.h ../../skeleton.h ../../utils.h

LLRBTree-test: LLRBTree-test.o LLRBTree.o tree-utils.o ../../utils.o ../../ll/ll.o

LLRBTree-test.o: LLRBTree-test.c trees.h ../../skeleton.h ../../utils.h ../../ll/ll.h

LLRBTree.o: LLRBTree.c trees.h ../../skeleton.h ../../utils.h ../../ll/ll.h

tree-utils.o: tree-utils.c trees.h ../../skeleton.h


.PHONY: clean
clean:
	rm -f *.o *~ LBBT-test BTree-test LLRBTree-test

.PHONY: all
all: clean LBBT-test BTree-test LLRBTree-test
