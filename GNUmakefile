include Makefile

SRCDIR = $(realpath $(dir $(firstword $(MAKEFILE_LIST))))

DEPS_ROOT = $(SRCDIR)/dependency/install
DEPS_PREFIX = $(DEPS_ROOT)/usr
DEPS_BIN = $(DEPS_PREFIX)/bin
DEPS_LIB = $(DEPS_PREFIX)/lib
DEPS_INC = $(DEPS_PREFIX)/include

LIBMUSL = musl-1.1.16
LIBMUSL_SHA1 = 5c2204b31b1ee08a01d4d3e34c6e46f6256bdac8

LIBNCURSES = ncurses-6.0
LIBNCURSES_SHA1 = acd606135a5124905da770803c05f1f20dd3b21c

LIBTERMKEY = libtermkey-0.19
LIBTERMKEY_SHA1 = a6b55687db1c16b64f2587a81bde602e73007add

LIBLUA = lua-5.3.3
LIBLUA_SHA1 = a0341bc3d1415b814cc738b2ec01ae56045d64ef
#LIBLUA = lua-5.2.4
#LIBLUA_SHA1 = ef15259421197e3d85b7d6e4871b8c26fd82c1cf
#LIBLUA = lua-5.1.5
#LIBLUA_SHA1 = b3882111ad02ecc6b972f8c1241647905cb2e3fc

LIBLPEG = lpeg-1.0.1
LIBLPEG_SHA1 = b2f81624e0ce9c99c0731287c3475fac1f1c0f50

LIBACL = acl-2.2.52
LIBACL_SHA1 = 537dddc0ee7b6aa67960a3de2d36f1e2ff2059d9

LIBATTR = attr-2.4.47
LIBATTR_SHA1 = 5060f0062baee6439f41a433325b8b3671f8d2d8

LIBNCURSES_CONFIG = --disable-database --with-fallbacks=st,st-256color,xterm,xterm-256color,vt100 \
	--with-shared --enable-widec --enable-ext-colors --with-termlib=tinfo \
	--without-ada --without-cxx --without-cxx-binding --without-manpages \
	--without-tests --without-progs --without-debug --without-profile \
	--without-cxx-shared --without-termlib --without--ticlib --disable-leaks

dependency/build:
	mkdir -p "$@"

dependency/sources:
	mkdir -p "$@"

# LIBMUSL

dependency/sources/$(LIBMUSL).tar.gz: | dependency/sources
	wget -c -O $@.part http://www.musl-libc.org/releases/$(LIBMUSL).tar.gz
	mv $@.part $@
	[ -z $(LIBMUSL_SHA1) ] || (echo '$(LIBMUSL_SHA1)  $@' | sha1sum -c)

dependency/build/$(LIBMUSL)-extract: dependency/sources/$(LIBMUSL).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/$(LIBMUSL)-configure: dependency/build/$(LIBMUSL)-extract
	# tweak musl gcc wrapper/spec file to support static PIE linking
	sed -i 's#%{pie:S}crt1.o#%{pie:%{static:rcrt1.o%s;:Scrt1.o%s};:crt1.o%s}#' $(dir $<)/$(LIBMUSL)/tools/musl-gcc.specs.sh
	cd $(dir $<)/$(LIBMUSL) && ./configure --prefix=$(DEPS_PREFIX) --syslibdir=$(DEPS_PREFIX)/lib
	touch $@

dependency/build/$(LIBMUSL)-build: dependency/build/$(LIBMUSL)-configure
	$(MAKE) -C $(dir $<)/$(LIBMUSL)
	touch $@

dependency/build/$(LIBMUSL)-install: dependency/build/$(LIBMUSL)-build
	$(MAKE) -C $(dir $<)/$(LIBMUSL) install
	touch $@

# LIBNCURSES

dependency/sources/$(LIBNCURSES).tar.gz: | dependency/sources
	wget -c -O $@.part http://ftp.gnu.org/gnu/ncurses/$(LIBNCURSES).tar.gz
	mv $@.part $@
	[ -z $(LIBNCURSES_SHA1) ] || (echo '$(LIBNCURSES_SHA1)  $@' | sha1sum -c)

dependency/build/$(LIBNCURSES)-extract: dependency/sources/$(LIBNCURSES).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/$(LIBNCURSES)-configure: dependency/build/$(LIBNCURSES)-extract
	cd $(dir $<)/$(LIBNCURSES) && ./configure --prefix=/usr --libdir=/usr/lib $(LIBNCURSES_CONFIG)
	touch $@

dependency/build/$(LIBNCURSES)-build: dependency/build/$(LIBNCURSES)-configure
	$(MAKE) -C $(dir $<)/$(LIBNCURSES)
	touch $@

dependency/build/$(LIBNCURSES)-install: dependency/build/$(LIBNCURSES)-build
	$(MAKE) -C $(dir $<)/$(LIBNCURSES) install.libs DESTDIR=$(DEPS_ROOT)
	touch $@

# LIBTERMKEY

dependency/sources/$(LIBTERMKEY).tar.gz: | dependency/sources
	wget -c -O $@.part http://www.leonerd.org.uk/code/libtermkey/$(LIBTERMKEY).tar.gz
	mv $@.part $@
	[ -z $(LIBTERMKEY_SHA1) ] || (echo '$(LIBTERMKEY_SHA1)  $@' | sha1sum -c)

dependency/build/$(LIBTERMKEY)-extract: dependency/sources/$(LIBTERMKEY).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/$(LIBTERMKEY)-build: dependency/build/$(LIBTERMKEY)-extract dependency/build/$(LIBNCURSES)-install
	# TODO no sane way to avoid pkg-config and specify LDFLAGS?
	sed -i 's/LDFLAGS+=-lncurses$$/LDFLAGS+=-lncursesw/g' $(dir $<)/$(LIBTERMKEY)/Makefile
	$(MAKE) -C $(dir $<)/$(LIBTERMKEY) PREFIX=/usr termkey.h libtermkey.la
	touch $@

dependency/build/$(LIBTERMKEY)-install: dependency/build/$(LIBTERMKEY)-build
	$(MAKE) -C $(dir $<)/$(LIBTERMKEY) PREFIX=/usr DESTDIR=$(DEPS_ROOT) install-inc install-lib
	touch $@

# LIBLUA

dependency/sources/$(LIBLUA).tar.gz: | dependency/sources
	wget -c -O $@.part http://www.lua.org/ftp/$(LIBLUA).tar.gz
	mv $@.part $@
	[ -z $(LIBLUA_SHA1) ] || (echo '$(LIBLUA_SHA1)  $@' | sha1sum -c)

dependency/build/$(LIBLUA)-extract: dependency/sources/$(LIBLUA).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/$(LIBLUA)-build: dependency/build/$(LIBLUA)-extract
	$(MAKE) -C $(dir $<)/$(LIBLUA)/src all CC=$(CC) MYCFLAGS="-DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2 -DLUA_COMPAT_ALL -DLUA_USE_POSIX -DLUA_USE_DLOPEN -fPIC" MYLIBS="-Wl,-E -ldl -lm"
	#$(MAKE) -C $(dir $<)/$(LIBLUA) posix CC=$(CC)
	touch $@

dependency/build/$(LIBLUA)-install: dependency/build/$(LIBLUA)-build
	$(MAKE) -C $(dir $<)/$(LIBLUA) INSTALL_TOP=$(DEPS_PREFIX) install
	touch $@

# LIBLPEG

dependency/sources/$(LIBLPEG).tar.gz: | dependency/sources
	wget -c -O $@.part http://www.inf.puc-rio.br/~roberto/lpeg/$(LIBLPEG).tar.gz
	mv $@.part $@
	[ -z $(LIBLPEG_SHA1) ] || (echo '$(LIBLPEG_SHA1)  $@' | sha1sum -c)

dependency/build/$(LIBLPEG)-extract: dependency/sources/$(LIBLPEG).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/$(LIBLPEG)-build: dependency/build/$(LIBLPEG)-extract dependency/build/$(LIBLUA)-extract
	# creating a shared object fails in Cygwin, we do not need it thus ignore the error
	cd $(dir $<)/$(LIBLPEG) && $(MAKE) LUADIR="../$(LIBLUA)/src" || true
	cd $(dir $<)/$(LIBLPEG) && ar rcu liblpeg.a lpvm.o lpcap.o lptree.o lpcode.o lpprint.o && ranlib liblpeg.a
	touch $@

dependency/build/$(LIBLPEG)-install: dependency/build/$(LIBLPEG)-build
	cd $(dir $<)/$(LIBLPEG) && cp liblpeg.a $(DEPS_LIB)
	touch $@

# LIBATTR

dependency/sources/$(LIBATTR).tar.gz: | dependency/sources
	wget -c -O $@.part https://download.savannah.gnu.org/releases/attr/$(LIBATTR).src.tar.gz
	mv $@.part $@
	[ -z $(LIBATTR_SHA1) ] || (echo '$(LIBATTR_SHA1)  $@' | sha1sum -c)

dependency/build/$(LIBATTR)-extract: dependency/sources/$(LIBATTR).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/$(LIBATTR)-configure: dependency/build/$(LIBATTR)-extract
	cd $(dir $<)/$(LIBATTR) && ./configure --prefix=/usr --libdir=/usr/lib
	touch $@

dependency/build/$(LIBATTR)-build: dependency/build/$(LIBATTR)-configure
	sed -i -e '/__BEGIN_DECLS/ c #ifdef __cplusplus\nextern "C" {\n#endif' \
		-e '/__END_DECLS/ c #ifdef __cplusplus\n}\n#endif' \
		-e 's/__THROW//' $(dir $<)/$(LIBATTR)/include/xattr.h
	$(MAKE) -C $(dir $<)/$(LIBATTR) libattr CC=$(CC)
	touch $@

dependency/build/$(LIBATTR)-install: dependency/build/$(LIBATTR)-build
	$(MAKE) -C $(dir $<)/$(LIBATTR)/libattr DESTDIR=$(DEPS_ROOT) install-dev
	$(MAKE) -C $(dir $<)/$(LIBATTR)/include DESTDIR=$(DEPS_ROOT) install-dev
	touch $@

# LIBACL

dependency/sources/$(LIBACL).tar.gz: | dependency/sources
	wget -c -O $@.part https://download.savannah.gnu.org/releases/acl/$(LIBACL).src.tar.gz
	mv $@.part $@
	[ -z $(LIBACL_SHA1) ] || (echo '$(LIBACL_SHA1)  $@' | sha1sum -c)

dependency/build/$(LIBACL)-extract: dependency/sources/$(LIBACL).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/$(LIBACL)-configure: dependency/build/$(LIBACL)-extract dependency/build/$(LIBATTR)-install
	cd $(dir $<)/$(LIBACL) && ./configure --prefix=/usr --libdir=/usr/lib --libexecdir=/usr/lib
	touch $@

dependency/build/$(LIBACL)-build: dependency/build/$(LIBACL)-configure
	$(MAKE) -C $(dir $<)/$(LIBACL) include libacl
	touch $@

dependency/build/$(LIBACL)-install: dependency/build/$(LIBACL)-build
	$(MAKE) -C $(dir $<)/$(LIBACL)/libacl DESTDIR=$(DEPS_ROOT) install-dev
	$(MAKE) -C $(dir $<)/$(LIBACL)/include DESTDIR=$(DEPS_ROOT) install-dev
	touch $@

# COMMON

dependencies-common: dependency/build/$(LIBTERMKEY)-install dependency/build/$(LIBLUA)-install dependency/build/$(LIBLPEG)-install

dependency/build/local: dependencies-common
	touch $@

dependency/build/standalone: dependency/build/$(LIBNCURSES)-install dependency/build/$(LIBACL)-install dependencies-common
	touch $@

dependencies-clean:
	rm -f dependency/build/$(LIBMUSL)-install
	rm -rf dependency/build/*curses*
	rm -rf dependency/build/libtermkey*
	rm -rf dependency/build/lua*
	rm -rf dependency/build/lpeg*
	rm -rf dependency/build/attr*
	rm -rf dependency/build/acl*
	rm -f dependency/build/local
	rm -f dependency/build/standalone
	rm -rf dependency/install
	rm -f config.mk

dependencies-local:
	[ ! -e dependency/build/standalone ] || $(MAKE) dependencies-clean
	mkdir -p dependency/build
	[ -e dependency/build/$(LIBNCURSES)-install ] || touch \
		dependency/build/$(LIBNCURSES)-extract \
		dependency/build/$(LIBNCURSES)-configure \
		dependency/build/$(LIBNCURSES)-build \
		dependency/build/$(LIBNCURSES)-install
	$(MAKE) dependency/build/local

local: clean
	./configure --environment-only
	$(MAKE) dependencies-local
	./configure CFLAGS="-I$(DEPS_INC)" LDFLAGS="-L$(DEPS_LIB)" LD_LIBRARY_PATH="$(DEPS_LIB)"
	$(MAKE)
	@echo Run with: LD_LIBRARY_PATH=$(DEPS_LIB) ./vis

standalone: clean
	[ ! -e dependency/build/local ] || $(MAKE) dependencies-clean
	./configure CFLAGS="-I$(DEPS_INC)" LDFLAGS="-L$(DEPS_LIB)" --environment-only --static
	CFLAGS="$(CFLAGS)" $(MAKE) dependency/build/$(LIBMUSL)-install
	PATH=$(DEPS_BIN):$$PATH PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR= CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" $(MAKE) \
		CC=musl-gcc dependency/build/standalone
	PATH=$(DEPS_BIN):$$PATH PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR= ./configure --static \
		CFLAGS="-I$(DEPS_INC) --static -Wl,--as-needed" LDFLAGS="-L$(DEPS_LIB)" CC=musl-gcc
	PATH=$(DEPS_BIN):$$PATH $(MAKE)

single: standalone
	cp vis-single.sh vis-single
	strip vis
	strip vis-menu
	tar c $(EXECUTABLES) lua/ | gzip -9 >> vis-single

.PHONY: standalone local dependencies-common dependencies-local dependencies-clean
