CC = gcc
CXX = g++

INCLUDES =

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

driver: driver.o ../group_manager/trees/tree-utils.o ../group_manager/trees/LBBT.o ../ll/ll.o ../group_manager/multicast/multicast.o ../users/user.o

driver.o: driver.c ../group_manager/trees/trees.h ../ll/ll.h ../group_manager/multicast/multicast.h ../users/user.h

.PHONY: clean
clean:
	rm -f *.o a.out driver *~ #*

.PHONY: all
all: clean driver
