include Makefile

SRCDIR = $(realpath $(dir $(firstword $(MAKEFILE_LIST))))

DEPS_ROOT = $(SRCDIR)/dependency/install
DEPS_PREFIX = $(DEPS_ROOT)/usr
DEPS_BIN = $(DEPS_PREFIX)/bin
DEPS_LIB = $(DEPS_PREFIX)/lib
DEPS_INC = $(DEPS_PREFIX)/include

LIBMUSL = musl-1.1.16
LIBMUSL_SHA256 = 937185a5e5d721050306cf106507a006c3f1f86d86cd550024ea7be909071011

LIBNCURSES = ncurses-6.0
LIBNCURSES_SHA256 = f551c24b30ce8bfb6e96d9f59b42fbea30fa3a6123384172f9e7284bcf647260

LIBTERMKEY = libtermkey-0.19
LIBTERMKEY_SHA256 = c505aa4cb48c8fa59c526265576b97a19e6ebe7b7da20f4ecaae898b727b48b7

LIBLUA = lua-5.3.4
LIBLUA_SHA256 = f681aa518233bc407e23acf0f5887c884f17436f000d453b2491a9f11a52400c
#LIBLUA = lua-5.2.4
#LIBLUA_SHA256 = b9e2e4aad6789b3b63a056d442f7b39f0ecfca3ae0f1fc0ae4e9614401b69f4b
#LIBLUA = lua-5.1.5
#LIBLUA_SHA256 = 2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333

LIBLPEG = lpeg-1.0.1
LIBLPEG_SHA256 = 62d9f7a9ea3c1f215c77e0cadd8534c6ad9af0fb711c3f89188a8891c72f026b

LIBATTR = attr-c1a7b53073202c67becf4df36cadc32ef4759c8a
LIBATTR_SHA256 = faf6e5cbfa71153bd1049206ca70690c5dc96e2ec3db50eae107092c3de900ca

LIBACL = acl-38f32ea1865bcc44185f4118fde469cb962cff68
LIBACL_SHA256 = 98598b0bb154ab294d9a695fd08b0e06516e770bbd1d78937905f0dd8ebe485c

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

dependency/sources/musl-%: | dependency/sources
	wget -c -O $@.part http://www.musl-libc.org/releases/$(LIBMUSL).tar.gz
	mv $@.part $@
	[ -z $(LIBMUSL_SHA256) ] || (echo '$(LIBMUSL_SHA256)  $@' | sha256sum -c)

dependency/build/libmusl-extract: dependency/sources/$(LIBMUSL).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/libmusl-configure: dependency/build/libmusl-extract
	# tweak musl gcc wrapper/spec file to support static PIE linking
	sed -i 's#%{pie:S}crt1.o#%{pie:%{static:rcrt1.o%s;:Scrt1.o%s};:crt1.o%s}#' $(dir $<)/$(LIBMUSL)/tools/musl-gcc.specs.sh
	cd $(dir $<)/$(LIBMUSL) && ./configure --prefix=$(DEPS_PREFIX) --syslibdir=$(DEPS_PREFIX)/lib
	touch $@

dependency/build/libmusl-build: dependency/build/libmusl-configure
	$(MAKE) -C $(dir $<)/$(LIBMUSL)
	touch $@

dependency/build/libmusl-install: dependency/build/libmusl-build
	$(MAKE) -C $(dir $<)/$(LIBMUSL) install
	touch $@

# LIBNCURSES

dependency/sources/ncurses-%: | dependency/sources
	wget -c -O $@.part http://ftp.gnu.org/gnu/ncurses/$(LIBNCURSES).tar.gz
	mv $@.part $@
	[ -z $(LIBNCURSES_SHA256) ] || (echo '$(LIBNCURSES_SHA256)  $@' | sha256sum -c)

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

# LIBTERMKEY

dependency/sources/libtermkey-%: | dependency/sources
	wget -c -O $@.part http://www.leonerd.org.uk/code/libtermkey/$(LIBTERMKEY).tar.gz
	mv $@.part $@
	[ -z $(LIBTERMKEY_SHA256) ] || (echo '$(LIBTERMKEY_SHA256)  $@' | sha256sum -c)

dependency/build/libtermkey-extract: dependency/sources/$(LIBTERMKEY).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/libtermkey-build: dependency/build/libtermkey-extract dependency/build/libncurses-install
	# TODO no sane way to avoid pkg-config and specify LDFLAGS?
	sed -i 's/LDFLAGS+=-lncurses$$/LDFLAGS+=-lncursesw/g' $(dir $<)/$(LIBTERMKEY)/Makefile
	$(MAKE) -C $(dir $<)/$(LIBTERMKEY) PREFIX=/usr termkey.h libtermkey.la
	touch $@

dependency/build/libtermkey-install: dependency/build/libtermkey-build
	$(MAKE) -C $(dir $<)/$(LIBTERMKEY) PREFIX=/usr DESTDIR=$(DEPS_ROOT) install-inc install-lib
	touch $@

# LIBLUA

dependency/sources/lua-%.tar.gz: | dependency/sources
	wget -c -O $@.part http://www.lua.org/ftp/$(LIBLUA).tar.gz
	mv $@.part $@
	[ -z $(LIBLUA_SHA256) ] || (echo '$(LIBLUA_SHA256)  $@' | sha256sum -c)

dependency/build/liblua-extract: dependency/sources/$(LIBLUA).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/liblua-build: dependency/build/liblua-extract
	$(MAKE) -C $(dir $<)/$(LIBLUA)/src all CC=$(CC) MYCFLAGS="-DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2 -DLUA_COMPAT_ALL -DLUA_USE_POSIX -DLUA_USE_DLOPEN -fPIC" MYLIBS="-Wl,-E -ldl -lm"
	#$(MAKE) -C $(dir $<)/$(LIBLUA) posix CC=$(CC)
	touch $@

dependency/build/liblua-install: dependency/build/liblua-build
	$(MAKE) -C $(dir $<)/$(LIBLUA) INSTALL_TOP=$(DEPS_PREFIX) install
	touch $@

# LIBLPEG

dependency/sources/lpeg-%: | dependency/sources
	wget -c -O $@.part http://www.inf.puc-rio.br/~roberto/lpeg/$(LIBLPEG).tar.gz
	mv $@.part $@
	[ -z $(LIBLPEG_SHA256) ] || (echo '$(LIBLPEG_SHA256)  $@' | sha256sum -c)

dependency/build/liblpeg-extract: dependency/sources/$(LIBLPEG).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	touch $@

dependency/build/liblpeg-build: dependency/build/liblpeg-extract dependency/build/liblua-extract
	# creating a shared object fails in Cygwin, we do not need it thus ignore the error
	cd $(dir $<)/$(LIBLPEG) && $(MAKE) LUADIR="../$(LIBLUA)/src" || true
	cd $(dir $<)/$(LIBLPEG) && ar rcu liblpeg.a lpvm.o lpcap.o lptree.o lpcode.o lpprint.o && ranlib liblpeg.a
	touch $@

dependency/build/liblpeg-install: dependency/build/liblpeg-build
	cd $(dir $<)/$(LIBLPEG) && cp liblpeg.a $(DEPS_LIB)
	touch $@

# LIBATTR

dependency/sources/attr-%.tar.gz: | dependency/sources
	wget -c -O $@.part http://git.savannah.gnu.org/cgit/attr.git/snapshot/$(LIBATTR).tar.gz
	mv $@.part $@
	[ -z $(LIBATTR_SHA256) ] || (echo '$(LIBATTR_SHA256)  $@' | sha256sum -c)

dependency/build/libattr-extract: dependency/sources/$(LIBATTR).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	cd $(dir $@)/$(LIBATTR) && ./autogen.sh
	touch $@

dependency/build/libattr-configure: dependency/build/libattr-extract
	cd $(dir $<)/$(LIBATTR) && ./configure --prefix=/usr --libdir=/usr/lib --with-sysroot=$(DEPS_ROOT)
	touch $@

dependency/build/libattr-build: dependency/build/libattr-configure
	$(MAKE) -C $(dir $<)/$(LIBATTR)
	touch $@

dependency/build/libattr-install: dependency/build/libattr-build
	$(MAKE) -C $(dir $<)/$(LIBATTR) DESTDIR=$(DEPS_ROOT) install
	touch $@

# LIBACL

dependency/sources/acl-%.tar.gz: | dependency/sources
	wget -c -O $@.part http://git.savannah.gnu.org/cgit/acl.git/snapshot/$(LIBACL).tar.gz
	mv $@.part $@
	[ -z $(LIBACL_SHA256) ] || (echo '$(LIBACL_SHA256)  $@' | sha256sum -c)

dependency/build/libacl-extract: dependency/sources/$(LIBACL).tar.gz | dependency/build
	tar xzf $< -C $(dir $@)
	cd $(dir $@)/$(LIBACL) && ./autogen.sh
	touch $@

dependency/build/libacl-configure: dependency/build/libacl-extract dependency/build/libattr-install
	cd $(dir $<)/$(LIBACL) && ./configure --prefix=/usr --libdir=/usr/lib --with-sysroot=$(DEPS_ROOT)
	touch $@

dependency/build/libacl-build: dependency/build/libacl-configure
	$(MAKE) -C $(dir $<)/$(LIBACL)
	touch $@

dependency/build/libacl-install: dependency/build/libacl-build
	$(MAKE) -C $(dir $<)/$(LIBACL) DESTDIR=$(DEPS_ROOT) install
	touch $@

# COMMON

dependencies-common: dependency/build/libtermkey-install dependency/build/liblua-install dependency/build/liblpeg-install

dependency/build/local: dependencies-common
	touch $@

dependency/build/standalone: dependency/build/libncurses-install dependency/build/libacl-install dependencies-common
	touch $@

dependencies-clean:
	rm -f dependency/build/libmusl-install
	rm -rf dependency/build/*curses*
	rm -rf dependency/build/libtermkey*
	rm -rf dependency/build/*lua*
	rm -rf dependency/build/*lpeg*
	rm -rf dependency/build/*attr*
	rm -rf dependency/build/*acl*
	rm -f dependency/build/local
	rm -f dependency/build/standalone
	rm -rf dependency/install
	rm -f config.mk

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
	./configure --environment-only
	$(MAKE) dependencies-local
	./configure CFLAGS="-I$(DEPS_INC)" LDFLAGS="-L$(DEPS_LIB)" LD_LIBRARY_PATH="$(DEPS_LIB)"
	$(MAKE)
	@echo Run with: LD_LIBRARY_PATH=$(DEPS_LIB) ./vis

standalone: clean
	[ ! -e dependency/build/local ] || $(MAKE) dependencies-clean
	./configure CFLAGS="-I$(DEPS_INC)" LDFLAGS="-L$(DEPS_LIB)" --environment-only --static
	CFLAGS="$(CFLAGS)" $(MAKE) dependency/build/libmusl-install
	PATH=$(DEPS_BIN):$$PATH PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR= CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" $(MAKE) \
		CC=musl-gcc dependency/build/standalone
	PATH=$(DEPS_BIN):$$PATH PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR= ./configure --static \
		CFLAGS="-I$(DEPS_INC) --static -Wl,--as-needed" LDFLAGS="-L$(DEPS_LIB)" CC=musl-gcc
	PATH=$(DEPS_BIN):$$PATH $(MAKE)

single: standalone
	cp vis-single.sh vis-single
	for e in $(ELF); do \
		${STRIP} "$$e"; \
	done
	tar c $(EXECUTABLES) lua/ | gzip -9 >> vis-single

.PHONY: standalone local dependencies-common dependencies-local dependencies-clean
