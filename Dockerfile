# docker build -t vis .
# docker run -it --name vis vis
# docker cp . vis:/tmp/vis
# ./configure CC='cc --static'
# make
# docker cp vis:/tmp/vis/vis .
# make vis-single
# docker cp vis:/tmp/vis/vis-single .
FROM i386/alpine:3.6
ENV DIR /tmp/vis
WORKDIR $DIR
RUN apk update && apk add musl-dev fortify-headers gcc make libtermkey-dev ncurses-dev ncurses-static lua5.3-dev lua5.3-lpeg lua-lpeg-dev acl-dev libarchive-dev xz-dev xz bzip2-dev tar
RUN sed -i 's/Libs: /Libs: -L${INSTALL_CMOD} /' /usr/lib/pkgconfig/lua5.3.pc
RUN mv /usr/lib/lua/5.3/lpeg.a /usr/lib/lua/5.3/liblpeg.a
RUN sed -i 's/-ltermkey/-ltermkey -lunibilium/' /usr/lib/pkgconfig/termkey.pc
CMD ["/bin/sh"]
