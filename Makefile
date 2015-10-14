include config.mk

SRCDIR = $(realpath $(dir $(firstword $(MAKEFILE_LIST))))

ALL = *.c *.h config.mk Makefile LICENSE README.md vis.1

DEPS_ROOT = $(SRCDIR)/dependency/install
DEPS_PREFIX = $(DEPS_ROOT)/usr
DEPS_BIN = $(DEPS_PREFIX)/bin
DEPS_LIB = $(DEPS_PREFIX)/lib
DEPS_INC = $(DEPS_PREFIX)/include

LIBMUSL = musl-1.1.11
LIBMUSL_SHA1 = 28eb54aeb3d2929c1ca415bb7853127e09bdf246

LIBNCURSES = ncurses-6.0
LIBNCURSES_SHA1 = acd606135a5124905da770803c05f1f20dd3b21c

LIBTERMKEY = libtermkey-0.18
LIBTERMKEY_SHA1 = 0a78ba7aaa2f3b53f2273268366fef349c9be4ab

#LIBLUA = lua-5.3.1
#LIBLUA_SHA1 = 1676c6a041d90b6982db8cef1e5fb26000ab6dee
#LIBLUA = lua-5.2.4
#LIBLUA_SHA1 = ef15259421197e3d85b7d6e4871b8c26fd82c1cf
LIBLUA = lua-5.1.5
LIBLUA_SHA1 = b3882111ad02ecc6b972f8c1241647905cb2e3fc

LIBLPEG = lpeg-0.12.2
LIBLPEG_SHA1 = 69eda40623cb479b4a30fb3720302d3a75f45577

LIBNCURSES_CONFIG = --disable-database --with-fallbacks=st-256color,xterm-256color,vt100 \
	--with-shared --enable-widec --enable-ext-colors --with-termlib=tinfo \
	--without-ada --without-cxx --without-cxx-binding --without-manpages --without-progs \
	--without-tests --without-progs --without-debug --without-profile \
	--without-cxx-shared --without-termlib --without--ticlib

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
	@echo installing support files to ${DESTDIR}${SHAREPREFIX}
	@mkdir -p ${DESTDIR}${SHAREPREFIX}
	@cp -r lexers ${DESTDIR}${SHAREPREFIX}
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

dependency/sources/musl-%: | dependency/sources
	wget -c -O $@.part http://www.musl-libc.org/releases/$(LIBMUSL).tar.gz
	mv $@.part $@
	[ -z $(LIBMUSL_SHA1) ] || (echo '$(LIBMUSL_SHA1)  $@' | sha1sum -c)

dependency/sources/extract-libmusl: dependency/sources/$(LIBMUSL).tar.gz
	tar xzf $< -C $(dir $<)
	touch $@

dependency/sources/configure-libmusl: dependency/sources/extract-libmusl
	cd $(dir $<)/$(LIBMUSL) && ./configure --prefix=$(DEPS_PREFIX) --syslibdir=$(DEPS_PREFIX)/lib
	touch $@

dependency/sources/build-libmusl: dependency/sources/configure-libmusl
	make -C $(dir $<)/$(LIBMUSL)
	touch $@

dependency/sources/install-libmusl: dependency/sources/build-libmusl
	make -C $(dir $<)/$(LIBMUSL) install
	touch $@

dependency/sources/ncurses-%: | dependency/sources
	wget -c -O $@.part http://ftp.gnu.org/gnu/ncurses/$(LIBNCURSES).tar.gz
	mv $@.part $@
	[ -z $(LIBNCURSES_SHA1) ] || (echo '$(LIBNCURSES_SHA1)  $@' | sha1sum -c)

dependency/sources/extract-libncurses: dependency/sources/$(LIBNCURSES).tar.gz
	tar xzf $< -C $(dir $<)
	touch $@

dependency/sources/configure-libncurses: dependency/sources/extract-libncurses
	cd $(dir $<)/$(LIBNCURSES) && ./configure --prefix=/usr $(LIBNCURSES_CONFIG)
	touch $@

dependency/sources/build-libncurses: dependency/sources/configure-libncurses
	make -C $(dir $<)/$(LIBNCURSES)
	touch $@

dependency/sources/install-libncurses: dependency/sources/build-libncurses
	make -C $(dir $<)/$(LIBNCURSES) install.libs DESTDIR=$(DEPS_ROOT)
	touch $@

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

dependency/sources/lua-%: | dependency/sources
	wget -c -O $@.part http://www.lua.org/ftp/$(LIBLUA).tar.gz
	mv $@.part $@
	[ -z $(LIBLUA_SHA1) ] || (echo '$(LIBLUA_SHA1)  $@' | sha1sum -c)

dependency/sources/extract-liblua: dependency/sources/$(LIBLUA).tar.gz
	tar xzf $< -C $(dir $<)
	touch $@

dependency/sources/patch-liblua: dependency/sources/extract-liblua
	cd $(dir $<) && ([ -e lua-5.1.4-lpeg.patch ] || wget http://www.brain-dump.org/projects/vis/lua-5.1.4-lpeg.patch)
	cd $(dir $<)/$(LIBLUA) && patch -p1 < ../lua-5.1.4-lpeg.patch
	touch $@

dependency/sources/build-liblua: dependency/sources/patch-liblua dependency/sources/install-liblpeg
	#make -C $(dir $<)/$(LIBLUA)/src all CC=$(CC) MYCFLAGS="-DLUA_USE_POSIX -DLUA_USE_DLOPEN" MYLIBS="-Wl,-E -ldl -lncursesw"
	make -C $(dir $<)/$(LIBLUA) posix CC=$(CC)
	touch $@

dependency/sources/install-liblua: dependency/sources/build-liblua
	make -C $(dir $<)/$(LIBLUA) INSTALL_TOP=$(DEPS_PREFIX) install
	touch $@

dependency/sources/lpeg-%: | dependency/sources
	wget -c -O $@.part http://www.inf.puc-rio.br/~roberto/lpeg/$(LIBLPEG).tar.gz
	mv $@.part $@
	[ -z $(LIBLPEG_SHA1) ] || (echo '$(LIBLPEG_SHA1)  $@' | sha1sum -c)

dependency/sources/extract-liblpeg: dependency/sources/$(LIBLPEG).tar.gz
	tar xzf $< -C $(dir $<)
	touch $@

dependency/sources/build-liblpeg: dependency/sources/extract-liblpeg
	make -C $(dir $<)/$(LIBLPEG) LUADIR=../$(LIBLUA)/src CC=$(CC)
	touch $@

dependency/sources/install-liblpeg: dependency/sources/build-liblpeg dependency/sources/extract-liblua
	cp $(dir $<)/$(LIBLPEG)/*.o $(dir $<)/$(LIBLUA)/src
	touch $@

dependencies: dependency/sources/install-libtermkey dependency/sources/install-liblua dependency/sources/install-liblpeg

dependencies-full: dependency/sources/install-libncurses dependencies

local: dependencies
	CFLAGS="$(CFLAGS) -I$(DEPS_INC)" LDFLAGS="$(LDFLAGS) -L$(DEPS_LIB)" make
	@echo Run with: LD_LIBRARY_PATH=$(DEPS_LIB) ./vis

standalone: dependency/sources/install-libmusl
	PATH=$(DEPS_BIN):$$PATH CC=musl-gcc PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR= CFLAGS=-I$(DEPS_INC)/ncursesw $(MAKE) dependencies-full
	PATH=$(DEPS_BIN):$$PATH CC=musl-gcc PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR= CFLAGS="--static -Wl,--as-needed -I$(DEPS_INC)/ncursesw" $(MAKE) CFLAGS_LIBS= debug

.PHONY: all clean dist install uninstall debug profile standalone dependencies dependencies-full directories
