# vis version
VERSION = 0.1

# optional features
CONFIG_LUA=1
CONFIG_ACL=0
CONFIG_SELINUX=0

# Customize below to fit your system

PREFIX ?= /usr/local
MANPREFIX ?= ${PREFIX}/share/man
SHAREPREFIX = ${PREFIX}/share/vis

LIBS ?= -lm -ldl -lc

CFLAGS_TERMKEY = $(shell pkg-config --cflags termkey 2> /dev/null || echo "")
LDFLAGS_TERMKEY = $(shell pkg-config --libs termkey 2> /dev/null || echo "-ltermkey")

CFLAGS_CURSES = $(shell pkg-config --cflags ncursesw 2> /dev/null || echo "-I/usr/include/ncursesw")
LDFLAGS_CURSES = $(shell pkg-config --libs ncursesw 2> /dev/null || echo "-lncursesw")

ifeq (${CONFIG_LUA},1)
	CFLAGS_LUA ?= $(shell pkg-config --cflags lua5.2 2> /dev/null || echo "-I/usr/include/lua5.2")
	LDFLAGS_LUA ?= $(shell pkg-config --libs lua5.2 2> /dev/null || echo "-llua")
endif

CFLAGS += -DCONFIG_LUA=${CONFIG_LUA}
CFLAGS += -DCONFIG_SELINUX=${CONFIG_SELINUX}
CFLAGS += -DCONFIG_ACL=${CONFIG_ACL}

ifeq (${CONFIG_ACL},1)
	LIBS += -lacl
endif

OS = $(shell uname)

ifeq (${OS},Linux)
	ifeq (${CONFIG_SELINUX},1)
		LIBS += -lselinux
	endif
else ifeq (${OS},Darwin)
	CFLAGS += -D_DARWIN_C_SOURCE
else ifeq (${OS},OpenBSD)
	CFLAGS += -D_BSD_SOURCE
else ifeq (${OS},FreeBSD)
	CFLAGS += -D_BSD_SOURCE
else ifeq (${OS},NetBSD)
	CFLAGS += -D_BSD_SOURCE
else ifeq (${OS},AIX)
	CFLAGS += -D_ALL_SOURCE
endif

CFLAGS_LIBS = $(CFLAGS_LUA) $(CFLAGS_TERMKEY) $(CFLAGS_CURSES)
LDFLAGS_LIBS = $(LDFLAGS_LUA) $(LDFLAGS_TERMKEY) $(LDFLAGS_CURSES) $(LIBS)

CFLAGS_VIS = $(CFLAGS_LIBS) -std=c99 -Os -DVERSION=\"${VERSION}\" -DNDEBUG -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2 -DLUA_COMPAT_ALL
LDFLAGS_VIS = $(LDFLAGS_LIBS)

DEBUG_CFLAGS_VIS = ${CFLAGS_VIS} -UNDEBUG -O0 -g -ggdb -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter

CC ?= cc
STRIP ?= strip

# Hardening
ifeq (${CC},gcc)
	CFLAGS += -fPIC -fstack-protector-all -D_FORTIFY_SOURCE=2
	LDFLAGS += -z now -z relro -pie
else ifeq (${CC},clang)
	CFLAGS += -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2
	LDFLAGS += -z now -z relro -pie
endif
