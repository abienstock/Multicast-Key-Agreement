CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = -g

LDLIBS = 

ll-test: ll-test.o ll.o

ll-test.o: ll-test.c ll.h

ll.o: ll.c ll.h


.PHONY: clean
clean:
	rm -f *.o *~

.PHONY: all
all: clean ll-test
