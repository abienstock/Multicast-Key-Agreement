CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

ll-test: ll-test.o ll.o ../utils.o

ll-test.o: ll-test.c ll.h ../utils.h

ll.o: ll.c ll.h ../utils.h


.PHONY: clean
clean:
	rm -f *.o *~ ll-test #*

.PHONY: all
all: clean ll-test
