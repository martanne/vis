include Makefile

TERMINFO_ENTRIES = st,st-256color,dvtm,dvtm-256color,xterm,xterm-256color,vt100,ansi

LIBTERMKEY = libtermkey-0.20
LIBTERMKEY_SHA256 = 6c0d87c94ab9915e76ecd313baec08dedf3bd56de83743d9aa923a081935d2f5

LIBLUA = lua-5.3.4
LIBLUA_SHA256 = f681aa518233bc407e23acf0f5887c884f17436f000d453b2491a9f11a52400c
#LIBLUA = lua-5.2.4
#LIBLUA_SHA256 = b9e2e4aad6789b3b63a056d442f7b39f0ecfca3ae0f1fc0ae4e9614401b69f4b
#LIBLUA = lua-5.1.5
#LIBLUA_SHA256 = 2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333

LIBLPEG = lpeg-1.0.1
LIBLPEG_SHA256 = 62d9f7a9ea3c1f215c77e0cadd8534c6ad9af0fb711c3f89188a8891c72f026b

SRCDIR = $(realpath $(dir $(firstword $(MAKEFILE_LIST))))

DEPS_ROOT = $(SRCDIR)/dependency/install
DEPS_PREFIX = $(DEPS_ROOT)/usr
DEPS_BIN = $(DEPS_PREFIX)/bin
DEPS_LIB = $(DEPS_PREFIX)/lib
DEPS_INC = $(DEPS_PREFIX)/include

dependency/build:
	mkdir -p "$@"

dependency/sources:
	mkdir -p "$@"

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

# COMMON

dependencies-common: dependency/build/libtermkey-install dependency/build/liblua-install dependency/build/liblpeg-install

dependency/build/local: dependencies-common
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

.PHONY: standalone local dependencies-common dependencies-local dependencies-clean
