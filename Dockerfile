# docker build -t vis .
# docker run -it --name vis vis
# docker cp . vis:/tmp/vis
# ./configure CC='cc --static'
# make
# docker cp vis:/tmp/vis/vis .
# make vis-single
# docker cp vis:/tmp/vis/vis-single .
FROM i386/alpine:3.8
ENV DIR /tmp/vis
WORKDIR $DIR
RUN apk update && apk add musl-dev fortify-headers gcc make libtermkey-dev \
	ncurses-dev ncurses-static lua5.3-dev lua5.3-lpeg lua-lpeg-dev \
	acl-dev xz-dev tar xz wget ca-certificates
RUN sed -i 's/Libs: /Libs: -L${INSTALL_CMOD} /' /usr/lib/pkgconfig/lua5.3.pc
RUN mv /usr/lib/lua/5.3/lpeg.a /usr/lib/lua/5.3/liblpeg.a
RUN sed -i 's/-ltermkey/-ltermkey -lunibilium/' /usr/lib/pkgconfig/termkey.pc
# TODO contribute a proper libuntar package to Alpine
RUN wget https://github.com/martanne/libuntar/tarball/3f5e915ad8e6c5faa8dc6b34532e32b519f278f3 -O libuntar.tar.gz && \
	tar xf libuntar.tar.gz && cd *-libuntar-* && \
	make && \
	mkdir -p /usr/local/include && \
	cp lib/libuntar.h /usr/local/include && \
	cp lib/libuntar.a /usr/local/lib
CMD ["/bin/sh"]
