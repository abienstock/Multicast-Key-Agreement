CC = gcc
CXX = g++

INCLUDES = -I../libbotan/include

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS = 

crypto_utils.o: crypto_utils.c ../utils.h

insecure.o: insecure.c crypto.h

secure.o: secure.c crypto.h ../utils.h

.PHONY: clean
clean:
	rm -f *.o a.out crypto_utils.o *~ #*

.PHONY: all
all: clean secure.o insecure.o crypto_utils.o
