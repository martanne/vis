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
	@${CC} ccan-config.c -o ccan-config && ./ccan-config > config.h

text: config.h text.c ../../text.c ../../text-util.c ../../text-motions.c ../../text-objects.c ../../text-regex.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${CFLAGS_EXTRA} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

buffer: config.h buffer.c ../../buffer.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${CFLAGS_EXTRA} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

map: config.h map.c ../../map.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${CFLAGS_EXTRA} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

array: config.h array.c ../../array.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${CFLAGS_EXTRA} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

debug: clean
	$(MAKE) CFLAGS_EXTRA='${CFLAGS_DEBUG}'

coverage: clean
	$(MAKE) CFLAGS_EXTRA='--coverage'

clean:
	@echo cleaning
	@rm -f text
	@rm -f buffer
	@rm -f map
	@rm -f array
	@rm -f *.gcov *.gcda *.gcno

.PHONY: clean debug coverage
