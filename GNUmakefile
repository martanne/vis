include Makefile

SRCDIR = $(realpath $(dir $(firstword $(MAKEFILE_LIST))))

DEPS_ROOT = $(SRCDIR)/dependency/install
DEPS_PREFIX = $(DEPS_ROOT)/usr
DEPS_BIN = $(DEPS_PREFIX)/bin
DEPS_LIB = $(DEPS_PREFIX)/lib
DEPS_INC = $(DEPS_PREFIX)/include

LIBMUSL = musl-1.1.14
LIBMUSL_SHA1 = b71208e87e66ac959d0e413dd444279d28a7bff1

LIBNCURSES = ncurses-6.0
LIBNCURSES_SHA1 = acd606135a5124905da770803c05f1f20dd3b21c

LIBTERMKEY = libtermkey-0.18
LIBTERMKEY_SHA1 = 0a78ba7aaa2f3b53f2273268366fef349c9be4ab

#LIBLUA = lua-5.3.1
#LIBLUA_SHA1 = 1676c6a041d90b6982db8cef1e5fb26000ab6dee
LIBLUA = lua-5.2.4
LIBLUA_SHA1 = ef15259421197e3d85b7d6e4871b8c26fd82c1cf
#LIBLUA = lua-5.1.5
#LIBLUA_SHA1 = b3882111ad02ecc6b972f8c1241647905cb2e3fc

LIBLPEG = lpeg-1.0.0
LIBLPEG_SHA1 = 64a0920c9243b624a277c987d2219b6c50c43971

LIBNCURSES_CONFIG = --disable-database --with-fallbacks=st,st-256color,xterm,xterm-256color,vt100 \
	--with-shared --enable-widec --enable-ext-colors --with-termlib=tinfo \
	--without-ada --without-cxx --without-cxx-binding --without-manpages --without-progs \
	--without-tests --without-progs --without-debug --without-profile \
	--without-cxx-shared --without-termlib --without--ticlib

dependency/build:
	mkdir -p "$@"

dependency/sources:
	mkdir -p "$@"

dependency/sources/musl-%: | dependency/sources
	wget -c -O $@.part http://www.musl-libc.org/releases/$(LIBMUSL).tar.gz
	mv $@.part $@
	[ -z $(LIBMUSL_SHA1) ] || (echo '$(LIBMUSL_SHA1)  $@' | sha1sum -c)

dependency/build/libmusl-extract: dependency/sources/$(LIBMUSL).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/libmusl-configure: dependency/build/libmusl-extract
	cd $(dir $<)/$(LIBMUSL) && ./configure --prefix=$(DEPS_PREFIX) --syslibdir=$(DEPS_PREFIX)/lib
	touch $@

dependency/build/libmusl-build: dependency/build/libmusl-configure
	$(MAKE) -C $(dir $<)/$(LIBMUSL)
	touch $@

dependency/build/libmusl-install: dependency/build/libmusl-build
	$(MAKE) -C $(dir $<)/$(LIBMUSL) install
	touch $@

dependency/sources/ncurses-%: | dependency/sources
	wget -c -O $@.part http://ftp.gnu.org/gnu/ncurses/$(LIBNCURSES).tar.gz
	mv $@.part $@
	[ -z $(LIBNCURSES_SHA1) ] || (echo '$(LIBNCURSES_SHA1)  $@' | sha1sum -c)

dependency/build/libncurses-extract: dependency/sources/$(LIBNCURSES).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/libncurses-configure: dependency/build/libncurses-extract
	cd $(dir $<)/$(LIBNCURSES) && ./configure --prefix=/usr --libdir=/usr/lib $(LIBNCURSES_CONFIG)
	touch $@

dependency/build/libncurses-build: dependency/build/libncurses-configure
	$(MAKE) -C $(dir $<)/$(LIBNCURSES)
	touch $@

dependency/build/libncurses-install: dependency/build/libncurses-build
	$(MAKE) -C $(dir $<)/$(LIBNCURSES) install.libs DESTDIR=$(DEPS_ROOT)
	touch $@

dependency/sources/libtermkey-%: | dependency/sources
	wget -c -O $@.part http://www.leonerd.org.uk/code/libtermkey/$(LIBTERMKEY).tar.gz
	mv $@.part $@
	[ -z $(LIBTERMKEY_SHA1) ] || (echo '$(LIBTERMKEY_SHA1)  $@' | sha1sum -c)

dependency/build/libtermkey-extract: dependency/sources/$(LIBTERMKEY).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/libtermkey-build: dependency/build/libtermkey-extract dependency/build/libncurses-install
	# TODO no sane way to avoid pkg-config and specify LDFLAGS?
	sed -i 's/LDFLAGS+=-lncurses$$/LDFLAGS+=-lncursesw/g' $(dir $<)/$(LIBTERMKEY)/Makefile
	$(MAKE) -C $(dir $<)/$(LIBTERMKEY) PREFIX=$(DEPS_PREFIX) termkey.h libtermkey.la
	touch $@

dependency/build/libtermkey-install: dependency/build/libtermkey-build
	$(MAKE) -C $(dir $<)/$(LIBTERMKEY) PREFIX=$(DEPS_PREFIX) install-inc install-lib
	touch $@

dependency/sources/lua-%.tar.gz: | dependency/sources
	wget -c -O $@.part http://www.lua.org/ftp/$(LIBLUA).tar.gz
	mv $@.part $@
	[ -z $(LIBLUA_SHA1) ] || (echo '$(LIBLUA_SHA1)  $@' | sha1sum -c)

dependency/build/liblua-extract: dependency/sources/$(LIBLUA).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/sources/lua-%-lpeg.patch: | dependency/sources
	wget -c -O $@.part http://www.brain-dump.org/projects/vis/$(LIBLUA)-lpeg.patch
	mv $@.part $@
	[ -z $(LIBLUA_LPEG_SHA1) ] || (echo '$(LIBLUA_LPEG_SHA1)  $@' | sha1sum -c)

dependency/build/liblua-patch: dependency/build/liblua-extract dependency/sources/$(LIBLUA)-lpeg.patch
	cd $(dir $<)/$(LIBLUA) && patch -p1 < ../../sources/$(LIBLUA)-lpeg.patch
	touch $@

dependency/build/liblua-build: dependency/build/liblua-patch dependency/build/liblpeg-install
	$(MAKE) -C $(dir $<)/$(LIBLUA)/src all CC=$(CC) MYCFLAGS="-DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2 -DLUA_COMPAT_ALL -DLUA_USE_POSIX -DLUA_USE_DLOPEN -fPIC" MYLIBS="-Wl,-E -ldl -lm"
	#$(MAKE) -C $(dir $<)/$(LIBLUA) posix CC=$(CC)
	touch $@

dependency/build/liblua-install: dependency/build/liblua-build
	$(MAKE) -C $(dir $<)/$(LIBLUA) INSTALL_TOP=$(DEPS_PREFIX) install
	touch $@

dependency/sources/lpeg-%: | dependency/sources
	wget -c -O $@.part http://www.inf.puc-rio.br/~roberto/lpeg/$(LIBLPEG).tar.gz
	mv $@.part $@
	[ -z $(LIBLPEG_SHA1) ] || (echo '$(LIBLPEG_SHA1)  $@' | sha1sum -c)

dependency/build/liblpeg-extract: dependency/sources/$(LIBLPEG).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/liblpeg-build: dependency/build/liblpeg-extract
	$(MAKE) -C $(dir $<)/$(LIBLPEG) LUADIR=../$(LIBLUA)/src CC=$(CC)
	touch $@

dependency/build/liblpeg-install: dependency/build/liblpeg-build dependency/build/liblua-extract
	cp $(dir $<)/$(LIBLPEG)/*.o $(dir $<)/$(LIBLUA)/src
	touch $@

dependencies-common: dependency/build/libtermkey-install dependency/build/liblua-install dependency/build/liblpeg-install

dependency/build/local: dependencies-common
	touch $@

dependency/build/standalone: dependency/build/libncurses-install dependencies-common
	touch $@

dependencies-clean:
	rm -f dependency/build/libmusl-install
	rm -rf dependency/build/*curses*
	rm -rf dependency/build/libtermkey*
	rm -rf dependency/build/*lua*
	rm -rf dependency/build/*lpeg*
	rm -f dependency/build/local
	rm -f dependency/build/standalone
	rm -rf dependency/install

dependencies-local:
	[ ! -e dependency/build/standalone ] || $(MAKE) dependencies-clean
	mkdir -p dependency/build
	[ -e dependency/build/libncurses-install ] || touch \
		dependency/build/libncurses-extract \
		dependency/build/libncurses-configure \
		dependency/build/libncurses-build \
		dependency/build/libncurses-install
	$(MAKE) dependency/build/local

local: clean
	$(MAKE) dependencies-local
	$(MAKE) CFLAGS="$(CFLAGS) -I$(DEPS_INC)" LDFLAGS="$(LDFLAGS) -L$(DEPS_LIB)" \
		CFLAGS_CURSES="-I/usr/include/ncursesw" LDFLAGS_CURSES="-lncursesw" \
		CFLAGS_TERMKEY=	LDFLAGS_TERMKEY=-ltermkey \
		CFLAGS_LUA="-DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2 -DLUA_COMPAT_ALL" \
		LDFLAGS_LUA="-llua -lm -ldl"
	@echo Run with: LD_LIBRARY_PATH=$(DEPS_LIB) ./vis

standalone: clean
	[ ! -e dependency/build/local ] || $(MAKE) dependencies-clean
	$(MAKE) dependency/build/libmusl-install
	PATH=$(DEPS_BIN):$$PATH PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR= $(MAKE) \
		CC=musl-gcc dependency/build/standalone
	PATH=$(DEPS_BIN):$$PATH PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR= $(MAKE) \
		CC=musl-gcc CFLAGS="--static -Wl,--as-needed" \
		CFLAGS_CURSES= LDFLAGS_CURSES="-lncursesw" \
		CFLAGS_TERMKEY= LDFLAGS_TERMKEY=-ltermkey \
		CFLAGS_LUA="-DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2 -DLUA_COMPAT_ALL" \
		LDFLAGS_LUA="-llua -lm -ldl" \
		CFLAGS_AUTO=-Os LDFLAGS_AUTO= \
		CONFIG_ACL=0 CFLAGS_ACL= LDFLAGS_ACL= \
		CONFIG_SELINUX=0 CFLAGS_SELINUX= LDFLAGS_SELINUX=

.PHONY: standalone local dependencies-common dependencies-local dependencies-clean
