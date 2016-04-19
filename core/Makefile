-include ../../config.mk

SRC = $(wildcard ccan/*/*.c)
CFLAGS += -I. -I../..

test: text buffer map array
	@./text
	@./buffer
	@./map
	@./array

config.h:
	@echo Generating ccan configuration header
	@${CC} ccan-config.c && ./a.out > config.h && rm -f a.out

text: config.h text.c ../../text.c ../../text-util.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

buffer: config.h buffer.c ../../buffer.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

map: config.h map.c ../../map.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

array: config.h array.c ../../array.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

debug: clean
	$(MAKE) CFLAGS_VIS='${DEBUG_CFLAGS_VIS}'

clean:
	@echo cleaning
	@rm -f text
	@rm -f buffer
	@rm -f map
	@rm -f array

.PHONY: clean debug
