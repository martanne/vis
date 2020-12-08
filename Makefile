-include config.mk

REGEX_SRC ?= text-regex.c

SRC = array.c \
	buffer.c \
	libutf.c \
	main.c \
	map.c \
	sam.c \
	text.c \
	text-common.c \
	text-io.c \
	text-iterator.c \
	text-motions.c \
	text-objects.c \
	text-util.c \
	ui-terminal.c \
	view.c \
	vis.c \
	vis-lua.c \
	vis-marks.c \
	vis-modes.c \
	vis-motions.c \
	vis-operators.c \
	vis-prompt.c \
	vis-registers.c \
	vis-text-objects.c \
	$(REGEX_SRC)

ELF = vis vis-menu vis-digraph
EXECUTABLES = $(ELF) vis-clipboard vis-complete vis-open

MANUALS = $(EXECUTABLES:=.1)

DOCUMENTATION = LICENSE README.md

VERSION = $(shell git describe --always --dirty 2>/dev/null || echo "v0.7-git")

CONFIG_HELP ?= 1
CONFIG_CURSES ?= 1
CONFIG_LUA ?= 1
CONFIG_LPEG ?= 0
CONFIG_TRE ?= 0
CONFIG_ACL ?= 0
CONFIG_SELINUX ?= 0

CFLAGS_STD ?= -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -DNDEBUG
CFLAGS_STD += -DVERSION=\"${VERSION}\"
LDFLAGS_STD ?= -lc

CFLAGS_LIBC ?= -DHAVE_MEMRCHR=0

CFLAGS_VIS = $(CFLAGS_AUTO) $(CFLAGS_TERMKEY) $(CFLAGS_CURSES) $(CFLAGS_ACL) \
	$(CFLAGS_SELINUX) $(CFLAGS_TRE) $(CFLAGS_LUA) $(CFLAGS_LPEG) $(CFLAGS_STD) \
	$(CFLAGS_LIBC)

CFLAGS_VIS += -DVIS_PATH=\"${SHAREPREFIX}/vis\"
CFLAGS_VIS += -DCONFIG_HELP=${CONFIG_HELP}
CFLAGS_VIS += -DCONFIG_CURSES=${CONFIG_CURSES}
CFLAGS_VIS += -DCONFIG_LUA=${CONFIG_LUA}
CFLAGS_VIS += -DCONFIG_LPEG=${CONFIG_LPEG}
CFLAGS_VIS += -DCONFIG_TRE=${CONFIG_TRE}
CFLAGS_VIS += -DCONFIG_SELINUX=${CONFIG_SELINUX}
CFLAGS_VIS += -DCONFIG_ACL=${CONFIG_ACL}

LDFLAGS_VIS = $(LDFLAGS_AUTO) $(LDFLAGS_TERMKEY) $(LDFLAGS_CURSES) $(LDFLAGS_ACL) \
	$(LDFLAGS_SELINUX) $(LDFLAGS_TRE) $(LDFLAGS_LUA) $(LDFLAGS_LPEG) $(LDFLAGS_STD)

STRIP?=strip
TAR?=tar
DOCKER?=docker

all: $(ELF)

config.h:
	cp config.def.h config.h

config.mk:
	@touch $@

vis: config.h config.mk *.c *.h
	${CC} ${CFLAGS} ${CFLAGS_VIS} ${CFLAGS_EXTRA} ${SRC} ${LDFLAGS} ${LDFLAGS_VIS} -o $@

vis-menu: vis-menu.c
	${CC} ${CFLAGS} ${CFLAGS_AUTO} ${CFLAGS_STD} ${CFLAGS_EXTRA} $< ${LDFLAGS} ${LDFLAGS_STD} ${LDFLAGS_AUTO} -o $@

vis-digraph: vis-digraph.c
	${CC} ${CFLAGS} ${CFLAGS_AUTO} ${CFLAGS_STD} ${CFLAGS_EXTRA} $< ${LDFLAGS} ${LDFLAGS_STD} ${LDFLAGS_AUTO} -o $@

vis-single-payload.inc: $(EXECUTABLES) lua/*
	for e in $(ELF); do \
		${STRIP} "$$e"; \
	done
	echo '#ifndef VIS_SINGLE_PAYLOAD_H' > $@
	echo '#define VIS_SINGLE_PAYLOAD_H' >> $@
	echo 'static unsigned char vis_single_payload[] = {' >> $@
	$(TAR) --mtime='2014-07-15 01:23Z' --owner=0 --group=0 --numeric-owner --mode='a+rX-w' -c \
		$(EXECUTABLES) $$(find lua -name '*.lua' | LC_ALL=C sort) | xz -T 1 | \
		od -t x1 -v | sed -e 's/^[0-9a-fA-F]\{1,\}//g' -e 's/\([0-9a-f]\{2\}\)/0x\1,/g' >> $@
	echo '};' >> $@
	echo '#endif' >> $@

vis-single: vis-single.c vis-single-payload.inc
	${CC} ${CFLAGS} ${CFLAGS_AUTO} ${CFLAGS_STD} ${CFLAGS_EXTRA} $< ${LDFLAGS} ${LDFLAGS_STD} ${LDFLAGS_AUTO} -luntar -llzma -o $@
	${STRIP} $@

docker-kill:
	-$(DOCKER) kill vis && $(DOCKER) wait vis

docker: docker-kill clean
	$(DOCKER) build -t vis .
	$(DOCKER) run --rm -d --name vis vis tail -f /dev/null
	$(DOCKER) exec vis apk update
	$(DOCKER) exec vis apk upgrade
	$(DOCKER) cp . vis:/build/vis
	$(DOCKER) exec -w /build/vis vis ./configure CC='cc --static' \
		--enable-acl \
		--enable-lua \
		--enable-lpeg-static
	$(DOCKER) exec -w /build/vis vis make VERSION="$(VERSION)" clean vis-single
	$(DOCKER) cp vis:/build/vis/vis-single vis
	$(DOCKER) kill vis

docker-clean: docker-kill clean
	-$(DOCKER) image rm vis

debug: clean
	@$(MAKE) CFLAGS_EXTRA='${CFLAGS_EXTRA} ${CFLAGS_DEBUG}'

profile: clean
	@$(MAKE) CFLAGS_AUTO='' LDFLAGS_AUTO='' CFLAGS_EXTRA='-pg -O2'

coverage: clean
	@$(MAKE) CFLAGS_EXTRA='--coverage'

test-update:
	git submodule init
	git submodule update --remote --rebase

test:
	[ -e test/Makefile ] || $(MAKE) test-update
	@$(MAKE) -C test

testclean:
	@echo cleaning the test artifacts
	[ ! -e test/Makefile ] || $(MAKE) -C test clean

clean:
	@echo cleaning
	@rm -f $(ELF) vis-single vis-single-payload.inc vis-*.tar.gz *.gcov *.gcda *.gcno

distclean: clean testclean
	@echo cleaning build configuration
	@rm -f config.h config.mk

dist: distclean
	@echo creating dist tarball
	@git archive --prefix=vis-${VERSION}/ -o vis-${VERSION}.tar.gz HEAD

man:
	@for m in ${MANUALS}; do \
		echo "Generating $$m"; \
		sed -e "s/VERSION/${VERSION}/" "man/$$m" | mandoc -W warning -T utf8 -T html -O man=%N.%S.html -O style=mandoc.css 1> "man/$$m.html" || true; \
	done

luadoc:
	@cd lua/doc && ldoc . && sed -e "s/RELEASE/${VERSION}/" -i index.html

luadoc-all:
	@cd lua/doc && ldoc -a . && sed -e "s/RELEASE/${VERSION}/" -i index.html

luacheck:
	@luacheck --config .luacheckrc lua test/lua | less -RFX

install: $(ELF)
	@echo installing executable files to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@for e in ${EXECUTABLES}; do \
		cp -f "$$e" ${DESTDIR}${PREFIX}/bin && \
		chmod 755 ${DESTDIR}${PREFIX}/bin/"$$e"; \
	done
	@test ${CONFIG_LUA} -eq 0 || { \
		echo installing support files to ${DESTDIR}${SHAREPREFIX}/vis; \
		mkdir -p ${DESTDIR}${SHAREPREFIX}/vis; \
		cp -r lua/* ${DESTDIR}${SHAREPREFIX}/vis; \
		rm -rf "${DESTDIR}${SHAREPREFIX}/vis/doc"; \
	}
	@echo installing documentation to ${DESTDIR}${DOCPREFIX}/vis
	@mkdir -p ${DESTDIR}${DOCPREFIX}/vis
	@for d in ${DOCUMENTATION}; do \
		cp "$$d" ${DESTDIR}${DOCPREFIX}/vis && \
		chmod 644 "${DESTDIR}${DOCPREFIX}/vis/$$d"; \
	done
	@echo installing manual pages to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@for m in ${MANUALS}; do \
		sed -e "s/VERSION/${VERSION}/" < "man/$$m" >  "${DESTDIR}${MANPREFIX}/man1/$$m" && \
		chmod 644 "${DESTDIR}${MANPREFIX}/man1/$$m"; \
	done

install-strip: install
	@echo stripping executables
	@for e in $(ELF); do \
		${STRIP} ${DESTDIR}${PREFIX}/bin/"$$e"; \
	done

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@for e in ${EXECUTABLES}; do \
		rm -f ${DESTDIR}${PREFIX}/bin/"$$e"; \
	done
	@echo removing documentation from ${DESTDIR}${DOCPREFIX}/vis
	@for d in ${DOCUMENTATION}; do \
		rm -f ${DESTDIR}${DOCPREFIX}/vis/"$$d"; \
	done
	@echo removing manual pages from ${DESTDIR}${MANPREFIX}/man1
	@for m in ${MANUALS}; do \
		rm -f ${DESTDIR}${MANPREFIX}/man1/"$$m"; \
	done
	@echo removing support files from ${DESTDIR}${SHAREPREFIX}/vis
	@rm -rf ${DESTDIR}${SHAREPREFIX}/vis

.PHONY: all clean testclean dist distclean install install-strip uninstall debug profile coverage test test-update luadoc luadoc-all luacheck man docker-kill docker docker-clean
