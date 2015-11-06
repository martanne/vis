# optional features
HAVE_ACL=0
HAVE_SELINUX=0

# vis version
RELEASE = HEAD
# try to get a tag and hash first
GITHASH = $(shell git log -1 --format='%h' 2>/dev/null)
GITTAG = $(shell git describe --abbrev=0 --tags 2>/dev/null)
ifneq ($(GITTAG),)
	# we have a tag and revcount from there
	GITREVCOUNT = $(shell git rev-list --count ${GITTAG}.. 2>/dev/null)
	VERSION = ${GITTAG}.r${GITREVCOUNT}.g${GITHASH}
else ifneq ($(GITHASH),)
	# we have no tags in git, so just use revision count an hash for now
	GITREVCOUNT = $(shell git rev-list --count HEAD)
	VERSION = 0.r${GITREVCOUNT}.g${GITHASH}
else
	# this is used when no git is available, e.g. for release tarball
	VERSION = ${RELEASE}
endif

# Customize below to fit your system

PREFIX ?= /usr/local
MANPREFIX = ${PREFIX}/share/man
SHAREPREFIX = ${PREFIX}/share/vis

CFLAGS_LUA = $(shell pkg-config --cflags lua5.1 2> /dev/null || echo "-I/usr/include/lua5.1")
CFLAGS_TERMKEY = $(shell pkg-config --cflags termkey 2> /dev/null || echo "")
CFLAGS_CURSES = $(shell pkg-config --cflags ncursesw 2> /dev/null || echo "-I/usr/include/ncursesw")

LDFLAGS_LUA = $(shell pkg-config --libs lua5.1 2> /dev/null || echo "-llua")
LDFLAGS_TERMKEY = $(shell pkg-config --libs termkey 2> /dev/null || echo "-ltermkey")
LDFLAGS_CURSES = $(shell pkg-config --libs ncursesw 2> /dev/null || echo "-lncursesw")

LIBS = -lm -ldl -lc
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
