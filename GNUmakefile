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
	$(MAKE) -C $(dir $<)/$(LIBMUSL)
	touch $@

dependency/sources/install-libmusl: dependency/sources/build-libmusl
	$(MAKE) -C $(dir $<)/$(LIBMUSL) install
	touch $@

dependency/sources/ncurses-%: | dependency/sources
	wget -c -O $@.part http://ftp.gnu.org/gnu/ncurses/$(LIBNCURSES).tar.gz
	mv $@.part $@
	[ -z $(LIBNCURSES_SHA1) ] || (echo '$(LIBNCURSES_SHA1)  $@' | sha1sum -c)

dependency/sources/extract-libncurses: dependency/sources/$(LIBNCURSES).tar.gz
	tar xzf $< -C $(dir $<)
	touch $@

dependency/sources/configure-libncurses: dependency/sources/extract-libncurses
	cd $(dir $<)/$(LIBNCURSES) && ./configure --prefix=/usr --libdir=/usr/lib $(LIBNCURSES_CONFIG)
	touch $@

dependency/sources/build-libncurses: dependency/sources/configure-libncurses
	$(MAKE) -C $(dir $<)/$(LIBNCURSES)
	touch $@

dependency/sources/install-libncurses: dependency/sources/build-libncurses
	$(MAKE) -C $(dir $<)/$(LIBNCURSES) install.libs DESTDIR=$(DEPS_ROOT)
	touch $@

dependency/sources/libtermkey-%: | dependency/sources
	wget -c -O $@.part http://www.leonerd.org.uk/code/libtermkey/$(LIBTERMKEY).tar.gz
	mv $@.part $@
	[ -z $(LIBTERMKEY_SHA1) ] || (echo '$(LIBTERMKEY_SHA1)  $@' | sha1sum -c)

dependency/sources/extract-libtermkey: dependency/sources/$(LIBTERMKEY).tar.gz
	tar xzf $< -C $(dir $<)
	touch $@

dependency/sources/build-libtermkey: dependency/sources/extract-libtermkey dependency/sources/install-libncurses
	# TODO no sane way to avoid pkg-config and specify LDFLAGS?
	sed -i 's/LDFLAGS+=-lncurses$$/LDFLAGS+=-lncursesw/g' $(dir $<)/$(LIBTERMKEY)/Makefile
	$(MAKE) -C $(dir $<)/$(LIBTERMKEY) PREFIX=$(DEPS_PREFIX) termkey.h libtermkey.la
	touch $@

dependency/sources/install-libtermkey: dependency/sources/build-libtermkey
	$(MAKE) -C $(dir $<)/$(LIBTERMKEY) PREFIX=$(DEPS_PREFIX) install-inc install-lib
	touch $@

dependency/sources/lua-%: | dependency/sources
	wget -c -O $@.part http://www.lua.org/ftp/$(LIBLUA).tar.gz
	mv $@.part $@
	[ -z $(LIBLUA_SHA1) ] || (echo '$(LIBLUA_SHA1)  $@' | sha1sum -c)

dependency/sources/extract-liblua: dependency/sources/$(LIBLUA).tar.gz
	tar xzf $< -C $(dir $<)
	touch $@

dependency/sources/patch-liblua: dependency/sources/extract-liblua
	cd $(dir $<) && ([ -e $(LIBLUA)-lpeg.patch ] || wget http://www.brain-dump.org/projects/vis/$(LIBLUA)-lpeg.patch)
	cd $(dir $<)/$(LIBLUA) && patch -p1 < ../$(LIBLUA)-lpeg.patch
	touch $@

dependency/sources/build-liblua: dependency/sources/patch-liblua dependency/sources/install-liblpeg
	$(MAKE) -C $(dir $<)/$(LIBLUA)/src all CC=$(CC) MYCFLAGS="-DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2 -DLUA_COMPAT_ALL -DLUA_USE_POSIX -DLUA_USE_DLOPEN -fPIC" MYLIBS="-Wl,-E -ldl -lm"
	#$(MAKE) -C $(dir $<)/$(LIBLUA) posix CC=$(CC)
	touch $@

dependency/sources/install-liblua: dependency/sources/build-liblua
	$(MAKE) -C $(dir $<)/$(LIBLUA) INSTALL_TOP=$(DEPS_PREFIX) install
	touch $@

dependency/sources/lpeg-%: | dependency/sources
	wget -c -O $@.part http://www.inf.puc-rio.br/~roberto/lpeg/$(LIBLPEG).tar.gz
	mv $@.part $@
	[ -z $(LIBLPEG_SHA1) ] || (echo '$(LIBLPEG_SHA1)  $@' | sha1sum -c)

dependency/sources/extract-liblpeg: dependency/sources/$(LIBLPEG).tar.gz
	tar xzf $< -C $(dir $<)
	touch $@

dependency/sources/build-liblpeg: dependency/sources/extract-liblpeg
	$(MAKE) -C $(dir $<)/$(LIBLPEG) LUADIR=../$(LIBLUA)/src CC=$(CC)
	touch $@

dependency/sources/install-liblpeg: dependency/sources/build-liblpeg dependency/sources/extract-liblua
	cp $(dir $<)/$(LIBLPEG)/*.o $(dir $<)/$(LIBLUA)/src
	touch $@

dependencies: dependency/sources/install-libtermkey dependency/sources/install-liblua dependency/sources/install-liblpeg

dependencies-full: dependency/sources/install-libncurses dependencies

local: clean dependencies
	$(MAKE) CFLAGS="$(CFLAGS) -I$(DEPS_INC)" LDFLAGS="$(LDFLAGS) -L$(DEPS_LIB)" \
		CFLAGS_CURSES="-I/usr/include/ncursesw" LDFLAGS_CURSES="-lncursesw" \
		CFLAGS_TERMKEY=	LDFLAGS_TERMKEY=-ltermkey \
		CFLAGS_LUA="-DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2 -DLUA_COMPAT_ALL" \
		LDFLAGS_LUA="-llua -lm -ldl"
	@echo Run with: LD_LIBRARY_PATH=$(DEPS_LIB) ./vis

standalone: clean dependency/sources/install-libmusl
	PATH=$(DEPS_BIN):$$PATH PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR= $(MAKE) \
		CC=musl-gcc dependencies-full
	PATH=$(DEPS_BIN):$$PATH PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR= $(MAKE) \
		CC=musl-gcc CFLAGS="--static -Wl,--as-needed" \
		CFLAGS_CURSES= LDFLAGS_CURSES="-lncursesw" \
		CFLAGS_TERMKEY= LDFLAGS_TERMKEY=-ltermkey \
		CFLAGS_LUA="-DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2 -DLUA_COMPAT_ALL" \
		LDFLAGS_LUA="-llua -lm -ldl" \
		CONFIG_ACL=0 CFLAGS_ACL= LDFLAGS_ACL= \
		CONFIG_SELINUX=0 CFLAGS_SELINUX= LDFLAGS_SELINUX=

.PHONY: standalone dependencies dependencies-full local