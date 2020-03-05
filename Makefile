CC = gcc

CFLAGS = -g -Wall

driver: driver.o

driver.o: driver.c

.PHONY: clean
clean:
	rm -f *.o a.out driver

.PHONY: all
all: clean driver