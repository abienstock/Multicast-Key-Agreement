CC = gcc
CXX = g++

INCLUDES = -Idriver/include

LDLIBS = -Ldriver/lib -lbotan-2

CFLAGS = -g -Wall -O2 $(INCLUDES)
CXXFLAGS = -g -Wall -O2 $(INCLUDES)

LDFLAGS = -g -lm -O2

LDLIBS =

SUBDIRS = driver ll group_manager/multicast group_manager/trees users
BUILDDIRS = $(SUBDIRS:%=build-%)
CLEANDIRS = $(SUBDIRS:%=clean-%)

all: clean $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%) all

driver:
	$(MAKE) -C driver all

clean: $(CLEANDIRS)
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean


.PHONY: subdirs $(SUBDIRS)
.PHONY: subdirs $(BUILDDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: all clean
