include config.mk

ALL = *.c *.h config.mk Makefile LICENSE README vis.1

all: vis

config.h:
	cp config.def.h config.h

vis: config.h config.mk *.c *.h
	@echo ${CC} ${CFLAGS} *.c ${LDFLAGS} -o $@
	@${CC} ${CFLAGS} *.c ${LDFLAGS} -o $@

debug: clean
	@$(MAKE) CFLAGS='${DEBUG_CFLAGS}'

profile: clean
	@$(MAKE) CFLAGS='${DEBUG_CFLAGS} -pg'

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
	git archive --format=tar.xz --prefix=vis-$(RELEASE)/ $(RELEASE) > vis-$(RELEASE).tar.xz

.PHONY: all clean dist install uninstall debug
