include config.mk

SRCDIR = $(realpath $(dir $(firstword $(MAKEFILE_LIST))))

ALL = *.c *.h config.mk Makefile LICENSE README.md vis.1

DEPS_ROOT = $(SRCDIR)/dependency/install
DEPS_PREFIX = $(DEPS_ROOT)/usr
DEPS_BIN = $(DEPS_PREFIX)/bin
DEPS_LIB = $(DEPS_PREFIX)/lib
DEPS_INC = $(DEPS_PREFIX)/include

LIBTERMKEY = libtermkey-0.18
LIBTERMKEY_SHA1 = 0a78ba7aaa2f3b53f2273268366fef349c9be4ab

all: vis

config.h:
	cp config.def.h config.h

vis: config.h config.mk *.c *.h
	@echo ${CC} ${CFLAGS} ${CFLAGS_VIS} *.c ${LDFLAGS} ${LDFLAGS_VIS} -o $@
	@${CC} ${CFLAGS} ${CFLAGS_VIS} *.c ${LDFLAGS} ${LDFLAGS_VIS} -o $@

debug: clean
	@$(MAKE) CFLAGS_VIS='${DEBUG_CFLAGS_VIS}'

profile: clean
	@$(MAKE) CFLAGS_VIS='${DEBUG_CFLAGS_VIS} -pg'

clean:
	@echo cleaning
	@rm -f vis vis-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p vis-${VERSION}
	@cp -R ${ALL} vis-${VERSION}
	@rm -f vis-${VERSION}/config.h
	@tar -cf vis-${VERSION}.tar vis-${VERSION}
	@gzip vis-${VERSION}.tar
	@rm -rf vis-${VERSION}

install: vis
	@echo stripping executable
	@${STRIP} vis
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f vis ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/vis
	@cp -f vis-open ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/vis-open
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < vis.1 > ${DESTDIR}${MANPREFIX}/man1/vis.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/vis.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/vis
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/vis.1

release:
	@git archive --prefix=vis-$(RELEASE)/ -o vis-$(RELEASE).tar.gz $(RELEASE)

dependency/sources:
	mkdir -p $@

dependency/sources/libtermkey-%: | dependency/sources
	wget -c -O $@.part http://www.leonerd.org.uk/code/libtermkey/$(LIBTERMKEY).tar.gz
	mv $@.part $@
	[ -z $(LIBTERMKEY_SHA1) ] || (echo '$(LIBTERMKEY_SHA1)  $@' | sha1sum -c)

dependency/sources/extract-libtermkey: dependency/sources/$(LIBTERMKEY).tar.gz
	tar xzf $< -C $(dir $<)
	touch $@

dependency/sources/build-libtermkey: dependency/sources/extract-libtermkey
	# TODO no sane way to avoid pkg-config and specify LDFLAGS?
	sed -i 's/LDFLAGS+=-lncurses$$/LDFLAGS+=-lncursesw/g' $(dir $<)/$(LIBTERMKEY)/Makefile
	make -C $(dir $<)/$(LIBTERMKEY) PREFIX=$(DEPS_PREFIX) termkey.h libtermkey.la
	touch $@

dependency/sources/install-libtermkey: dependency/sources/build-libtermkey
	make -C $(dir $<)/$(LIBTERMKEY) PREFIX=$(DEPS_PREFIX) install
	touch $@

dependencies: dependency/sources/install-libtermkey

local: dependencies
	CFLAGS="$(CFLAGS) -I$(DEPS_INC)" LDFLAGS="$(LDFLAGS) -L$(DEPS_LIB)" make CFLAGS_TERMKEY= LDFLAGS_TERMKEY=-ltermkey
	@echo Run with: LD_LIBRARY_PATH=$(DEPS_LIB) ./vis

.PHONY: all clean dist install uninstall debug profile dependencies local
