# optional features
HAVE_ACL=0
HAVE_SELINUX=0

# vis version
RELEASE = 0.0.0
# try to get a tag and hash first
GITHASH = $(shell git log -1 --format='%h' 2>/dev/null)
GITTAG = $(shell git describe --abbrev=0 --tags 2>/dev/null)
ifneq ($(GITTAG),)
	# we have a tag and revcount from there
	GITREVCOUNT = $(shell git rev-list --count ${GITTAG}.. 2>/dev/null)
	VERSION = ${GITTAG}.r${GITREVCOUNT}.g${GITHASH}
else ifneq ($(GITHASH),)
	# we have no tags in git, so just use revision count an hash for now
	GITREVCOUNT = $(shell git rev-list --count master)
	VERSION = 0.r${GITREVCOUNT}.g${GITHASH}
else
	# this is used when no git is available, e.g. for release tarball
	VERSION = ${RELEASE}
endif

# Customize below to fit your system

PREFIX ?= /usr/local
MANPREFIX = ${PREFIX}/share/man

INCS = -I.
LIBS = -lc -lncursesw -ltermkey

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
