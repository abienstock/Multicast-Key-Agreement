CC = gcc
CXX = g++

INCLUDES = 

LDLIBS = 

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

SUBDIRS = crypto driver ll group_manager/multicast group_manager/trees users
BUILDDIRS = $(SUBDIRS:%=build-%)
CLEANDIRS = $(SUBDIRS:%=clean-%)

all: clean libbotan $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%) all

libbotan:
	mkdir -p $@
	git clone https://github.com/randombit/botan.git $@ || (cd $@; git pull)
	cd $@; ./configure.py --prefix=$(CURDIR)/$@ --enable-modules=auto_rng,system_rng,hmac,mac,aead,rng,ffi,chacha --without-documentation && make && make install && (cd include; ln -sf botan-2/botan botan)
	cd $@; ./botan-test


driver:
	$(MAKE) -C driver all

clean: $(CLEANDIRS)
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean


.PHONY: subdirs $(SUBDIRS)
.PHONY: subdirs $(BUILDDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: all clean
