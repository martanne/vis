# vis version
VERSION = devel

# Customize below to fit your system

PREFIX ?= /usr/local
MANPREFIX = ${PREFIX}/share/man

INCS = -I.

ifndef PLATFORM_OS
        PLATFORM_OS=$(shell uname -s)
endif

ifeq ($(PLATFORM_OS),Darwin)
    LIBS = -lc -lncurses
else
    LIBS = -lc -lncursesw
endif

ifeq ($(shell uname),Darwin)
	LIBS = -lc -lncurses
	CFLAGS += -D_DARWIN_C_SOURCE
else ifeq ($(shell uname),OpenBSD)
	CFLAGS += -D_BSD_SOURCE
else ifeq ($(shell uname),FreeBSD)
	CFLAGS += -D_BSD_SOURCE
else ifeq ($(shell uname),NetBSD)
	LIBS = -lc -lcurses
	CFLAGS += -D_BSD_SOURCE
else ifeq ($(shell uname),SunOS)
	INCS += -I/usr/include/ncurses
else ifeq ($(shell uname),AIX)
	CFLAGS += -D_ALL_SOURCE
endif

CFLAGS += -std=c99 -Os ${INCS} -DVERSION=\"${VERSION}\" -DNDEBUG -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700

LDFLAGS += ${LIBS}

DEBUG_CFLAGS = ${CFLAGS} -UNDEBUG -O0 -g -ggdb -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter

CC ?= cc
STRIP ?= strip
