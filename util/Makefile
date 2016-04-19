-include ../../config.mk

DEPS_ROOT = ../../dependency/install
DEPS_PREFIX = $(DEPS_ROOT)/usr
DEPS_LIB = $(DEPS_PREFIX)/lib
DEPS_INC = $(DEPS_PREFIX)/include

CFLAGS_TERMKEY ?= $(shell pkg-config --cflags termkey || echo "-I/usr/local/include")
LDFLAGS_TERMKEY ?= $(shell pkg-config --libs termkey || echo "-ltermkey")

keys: keys.c
	@echo Compiling keys utility
	$(CC) $(CFLAGS_TERMKEY) keys.c $(LDFLAGS_TERMKEY) -o keys

keys-local: keys.c
	@echo Compiling keys utility locally
	$(CC) -I$(DEPS_INC) keys.c -L$(DEPS_LIB) $(LDFLAGS_TERMKEY) $(LDFLAGS_CURSES) -o keys

clean:
	@echo cleaning
	@rm -f keys

.PHONY: clean keys-local
