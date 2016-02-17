include ../../config.mk

SRC = $(wildcard ccan/*/*.c)
CFLAGS_VIS += -I. -I../..

test: text buffer map
	@./text
	@./buffer
	@./map

config.h:
	@echo Generating ccan configuration header
	@${CC} ccan-config.c && ./a.out > config.h && rm -f a.out

text: config.h text.c ../../text.c ../../text-util.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_VIS} $^ ${SRC} ${LDFLAGS} -o $@

buffer: config.h buffer.c ../../buffer.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_VIS} $^ ${SRC} ${LDFLAGS} -o $@

map: config.h map.c ../../map.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_VIS} $^ ${SRC} ${LDFLAGS} -o $@

debug: clean
	@$(MAKE) CFLAGS_VIS='${DEBUG_CFLAGS_VIS}'

clean:
	@echo cleaning
	@rm -f text
	@rm -f buffer
	@rm -f map

.PHONY: clean debug
