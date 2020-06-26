CC = gcc
CXX = g++

INCLUDES = -I../libbotan/include

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS = -L../libbotan/lib -lbotan-2

test_driver: test_driver.o ../group_manager/trees/tree-utils.o ../group_manager/trees/LBBT.o ../ll/ll.o ../group_manager/multicast/multicast.o ../users/user.o ../crypto/secure.o ../crypto/crypto_utils.o ../utils.o

test_driver.o: test_driver.c ../group_manager/trees/trees.h ../ll/ll.h ../group_manager/multicast/multicast.h ../users/user.h ../crypto/crypto.h ../utils.h

.PHONY: clean
clean:
	rm -f *.o a.out *driver *~

.PHONY: all

all: clean test_driver
