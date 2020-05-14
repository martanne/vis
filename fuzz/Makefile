-include ../../config.mk

ALL = text-fuzzer buffer-fuzzer
CC = afl-gcc
CFLAGS += -I. -I../.. -DBUFFER_SIZE=4 -DBLOCK_SIZE=4

test: $(ALL)

text-fuzzer: text-fuzzer.c fuzzer.h ../../text.c ../../text-util.c ../../text-motions.c ../../text-objects.c ../../text-regex.c
	@echo Compiling $@ binary
	${CC} ${CFLAGS} ${CFLAGS_STD} ${CFLAGS_LIBC} ${CFLAGS_EXTRA} ${filter %.c, $^} ${LDFLAGS} -o $@

buffer-fuzzer: buffer-fuzzer.c fuzzer.h ../../buffer.c
	@echo Compiling $@ binary
	@${CC} ${CFLAGS} ${CFLAGS_STD} ${CFLAGS_LIBC} ${CFLAGS_EXTRA} ${filter %.c, $^} ${LDFLAGS} -o $@

debug: clean
	$(MAKE) CFLAGS_EXTRA='${CFLAGS_EXTRA} ${CFLAGS_DEBUG}'

afl-fuzz-text: text-fuzzer
	@mkdir -p "results/$<"
	@afl-fuzz -i - -x "dictionaries/$<.dict" -o "results/$<" -- "./$<" || \
	 afl-fuzz -i "inputs/$<" -x "dictionaries/$<.dict" -o "results/$<" -- "./$<"

afl-fuzz-buffer: buffer-fuzzer
	@mkdir -p "results/$<"
	@afl-fuzz -i - -x "dictionaries/$<.dict" -o "results/$<" -- "./$<" || \
	 afl-fuzz -i "inputs/$<" -x "dictionaries/$<.dict" -o "results/$<" -- "./$<"

clean:
	@echo cleaning
	@rm -f $(ALL)

distclean: clean
	@rm -rf results/

.PHONY: clean distclean debug afl-fuzz-text afl-fuzz-buffer
