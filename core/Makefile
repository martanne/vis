-include ../../config.mk

ALL = buffer-test map-test array-test text-test
SRC = $(wildcard ccan/*/*.c)
CFLAGS += -I. -I../.. -DBUFFER_SIZE=4 -DBLOCK_SIZE=4

test: $(ALL)
	@./buffer-test
	@./map-test
	@./array-test
	@./text-test

config.h:
	@echo Generating ccan configuration header
	@${CC} ccan-config.c -o ccan-config && ./ccan-config > config.h

text-test: config.h text-test.c ../../text.c ../../text-util.c ../../text-motions.c ../../text-objects.c ../../text-regex.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${CFLAGS_LIBC} ${CFLAGS_EXTRA} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

buffer-test: config.h buffer-test.c ../../buffer.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${CFLAGS_LIBC} ${CFLAGS_EXTRA} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

map-test: config.h map-test.c ../../map.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${CFLAGS_LIBC} ${CFLAGS_EXTRA} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

array-test: config.h array-test.c ../../array.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${CFLAGS_LIBC} ${CFLAGS_EXTRA} ${filter %.c, $^} ${SRC} ${LDFLAGS} -o $@

debug: clean
	$(MAKE) CFLAGS_EXTRA='${CFLAGS_EXTRA} ${CFLAGS_DEBUG}'

coverage: clean
	$(MAKE) CFLAGS_EXTRA='--coverage'

asan: clean
	$(MAKE) CFLAGS_EXTRA='-fsanitize=address'

ubsan: clean
	$(MAKE) CFLAGS_EXTRA='-fsanitize=undefined'

msan: clean
	$(MAKE) CFLAGS_EXTRA='-g -fsanitize=memory'

valgrind: clean ${ALL}
	@for test in ${ALL}; do \
		valgrind --leak-check=full --log-file="$$test.valgrind" "./$$test"; \
		cat "$$test.valgrind"; \
		grep LEAK "$$test.valgrind" >/dev/null && exit 1 || true; \
	done

tis: clean
	$(MAKE) CC="tis-interpreter.sh --cc" CFLAGS='"${CFLAGS} ${CFLAGS_STD} -DHAVE_MEMRCHR=0 -DTIS_INTERPRETER=1"' CFLAGS_STD='' CFLAGS_LIBC='' LDFLAGS='#' $(ALL)

clean:
	@echo cleaning
	@rm -f ccan-config config.h
	@rm -f $(ALL)
	@rm -f *.gcov *.gcda *.gcno
	@rm -f *.valgrind

.PHONY: clean debug coverage tis valgrind asan ubsan msan
