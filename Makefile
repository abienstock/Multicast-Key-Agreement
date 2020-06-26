CC = gcc
CXX = g++

INCLUDES = 

LDLIBS = 

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

SUBDIRS = crypto ll group_manager/trees group_manager/multicast users driver
BUILDDIRS = $(SUBDIRS:%=build-%)
CLEANDIRS = $(SUBDIRS:%=clean-%)

utils.o: utils.c utils.h

all: clean libbotan utils.o $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%) all

libbotan:
	mkdir -p $@
	git clone https://github.com/randombit/botan.git $@ || (cd $@; git pull)
	cd $@; ./configure.py --prefix=$(CURDIR)/$@ --enable-modules=auto_rng,system_rng,hmac,mac,aead,rng,ffi,chacha --without-documentation && make && make install && (cd include; ln -sf botan-2/botan botan)
	cd $@; ./botan-test

clean:
	rm -rf *~ utils.o

clean_all: $(CLEANDIRS)
	rm -rf *~ utils.o
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean


.PHONY: subdirs $(SUBDIRS)
.PHONY: subdirs $(BUILDDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: all clean clean_all
