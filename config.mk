# optional features
HAVE_ACL=0
HAVE_SELINUX=0

# vis version
# we have no tags in git, so just use revision count an hash for now
GITREVCOUNT = "$(shell git rev-list --count master 2>/dev/null)"
GITHASH = "$(shell git log -1 --format="%h" 2>/dev/null)"
ifeq (${GITREVCOUNT},)
	VERSION = devel
else
	VERSION = "0.r${GITREVCOUNT}.g${GITHASH}"
endif

# Customize below to fit your system

PREFIX ?= /usr/local
MANPREFIX = ${PREFIX}/share/man

INCS = -I.
LIBS = -lc -lncursesw

OS = $(shell uname)

ifeq (${OS},Linux)
	ifeq (${HAVE_SELINUX},1)
		CFLAGS += -DHAVE_SELINUX
		LIBS += -lselinux
	endif
	ifeq (${HAVE_ACL},1)
		CFLAGS += -DHAVE_ACL
		LIBS += -lacl
	endif
else ifeq (${OS},Darwin)
	LIBS = -lc -lncurses
	CFLAGS += -D_DARWIN_C_SOURCE
else ifeq (${OS},OpenBSD)
	LIBS = -lc -lncurses
	CFLAGS += -D_BSD_SOURCE
else ifeq (${OS},FreeBSD)
	CFLAGS += -D_BSD_SOURCE
else ifeq (${OS},NetBSD)
	LIBS = -lc -lcurses
	CFLAGS += -D_BSD_SOURCE
else ifeq (${OS},SunOS)
	INCS += -I/usr/include/ncurses
else ifeq (${OS},AIX)
	CFLAGS += -D_ALL_SOURCE
endif

CFLAGS += -std=c99 -Os ${INCS} -DVERSION=\"${VERSION}\" -DNDEBUG -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700

LDFLAGS += ${LIBS}

DEBUG_CFLAGS = ${CFLAGS} -UNDEBUG -O0 -g -ggdb -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter

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
