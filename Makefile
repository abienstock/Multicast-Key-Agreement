CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

trees := trees/
multicast := multicast/
libs := $(trees) $(multicast)

driver: driver.o trees/tree-utils.o trees/LBBT.o trees/ll.o multicast/multicast.o

driver.o: driver.c trees/trees.h trees/ll.h multicast/multicast.h

.PHONY: clean
clean:
	rm -f *.o a.out driver

.PHONY: all
all: clean driver
