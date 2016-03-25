-include config.mk

# conditionally initialized, this is needed for standalone build
# with empty config.mk
PREFIX ?= /usr/local
MANPREFIX ?= ${PREFIX}/share/man
SHAREPREFIX ?= ${PREFIX}/share/vis

VERSION = $(shell git describe 2>/dev/null || echo "0.2")

CONFIG_LUA ?= 1
CONFIG_ACL ?= 0
CONFIG_SELINUX ?= 0

CFLAGS_STD ?= -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -DNDEBUG
LDFLAGS_STD ?= -lc

CFLAGS_VIS = $(CFLAGS_AUTO) $(CFLAGS_TERMKEY) $(CFLAGS_CURSES) $(CFLAGS_ACL) \
	$(CFLAGS_SELINUX) $(CFLAGS_LUA) $(CFLAGS_STD)

CFLAGS_VIS += -DVERSION=\"${VERSION}\"
CFLAGS_VIS += -DCONFIG_LUA=${CONFIG_LUA}
CFLAGS_VIS += -DCONFIG_SELINUX=${CONFIG_SELINUX}
CFLAGS_VIS += -DCONFIG_ACL=${CONFIG_ACL}

LDFLAGS_VIS = $(LDFLAGS_AUTO) $(LDFLAGS_TERMKEY) $(LDFLAGS_CURSES) $(LDFLAGS_ACL) \
	$(LDFLAGS_SELINUX) $(LDFLAGS_LUA) $(LDFLAGS_STD)

DEBUG_CFLAGS_VIS = ${CFLAGS_VIS} -UNDEBUG -O0 -g -ggdb -Wall -Wextra -pedantic \
	-Wno-missing-field-initializers -Wno-unused-parameter

STRIP?=strip


all: vis

config.h:
	cp config.def.h config.h

config.mk:
	@touch $@

vis: config.h config.mk *.c *.h
	${CC} ${CFLAGS} ${CFLAGS_VIS} *.c ${LDFLAGS} ${LDFLAGS_VIS} -o $@

debug: clean
	@$(MAKE) CFLAGS_VIS='${DEBUG_CFLAGS_VIS}'

profile: clean
	@$(MAKE) CFLAGS_VIS='${DEBUG_CFLAGS_VIS} -pg'

clean:
	@echo cleaning
	@rm -f vis vis-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@git archive --prefix=vis-${VERSION}/ -o vis-${VERSION}.tar.gz HEAD

install: vis
	@echo stripping executable
	@${STRIP} vis
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f vis ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/vis
	@cp -f vis-open ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/vis-open
	@cp -f vis-clipboard ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/vis-clipboard
	@echo installing support files to ${DESTDIR}${SHAREPREFIX}/vis
	@mkdir -p ${DESTDIR}${SHAREPREFIX}/vis
	@cp -r visrc.lua lexers ${DESTDIR}${SHAREPREFIX}/vis
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < vis.1 > ${DESTDIR}${MANPREFIX}/man1/vis.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/vis.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/vis
	@rm -f ${DESTDIR}${PREFIX}/bin/vis-open
	@rm -f ${DESTDIR}${PREFIX}/bin/vis-clipboard
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/vis.1
	@echo removing support files from ${DESTDIR}${SHAREPREFIX}/vis
	@rm -rf ${DESTDIR}${SHAREPREFIX}/vis

.PHONY: all clean dist install uninstall debug profile
