CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = -g

LDLIBS =

LBBT-test: LBBT-test.o LBBT.o ll.o tree_utils.o

LBBT-test.o: LBBT-test.c trees.h LBBT.o

LBBT.o: LBBT.c trees.h ll.h

tree_utils.o: tree_utils.c trees.h

ll-test: ll-test.o ll.o

ll-test.o: ll-test.c ll.h

ll.o: ll.c ll.h


.PHONY: clean
clean:
	rm -f *.o *~ ll-test LBBT-test

.PHONY: all
all: clean ll-test LBBT-test
