# Run 'make docker' to build a statically linked vis executable!
FROM i386/alpine:3.11
RUN apk update && apk upgrade && \
	apk add musl-dev fortify-headers gcc make libtermkey-dev \
	ncurses-dev ncurses-static lua5.3-dev lua5.3-lpeg lua-lpeg-dev \
	acl-static acl-dev xz-dev tar xz wget ca-certificates rsync
RUN sed -i 's/Libs: /Libs: -L${INSTALL_CMOD} /' /usr/lib/pkgconfig/lua5.3.pc
RUN mv /usr/lib/lua/5.3/lpeg.a /usr/lib/lua/5.3/liblpeg.a
RUN sed -i 's/-ltermkey/-ltermkey -lunibilium/' /usr/lib/pkgconfig/termkey.pc
# TODO contribute a proper libuntar package to Alpine
RUN mkdir -p /build
WORKDIR /build
RUN wget https://github.com/martanne/libuntar/tarball/7c7247b442b021588f6deba78b60ef3b05ab1e0c -O libuntar.tar.gz && \
	tar xf libuntar.tar.gz && cd *-libuntar-* && \
	make && \
	mkdir -p /usr/local/include && \
	cp lib/libuntar.h /usr/local/include && \
	cp lib/libuntar.a /usr/local/lib && \
	rm -rf /build/*-libuntar-*
CMD ["/bin/sh"]
