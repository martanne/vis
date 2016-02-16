include ../../config.mk

SRC = text.c ../../text.c ../../text-util.c
SRC += $(wildcard ccan/*/*.c)
CFLAGS_VIS += -I. -I../..

test: text
	@./text

config.h:
	@echo Generating ccan configuration header
	@${CC} ccan-config.c && ./a.out > config.h && rm -f a.out

text: config.h text.c ../../text.c
	@echo Compiling text binary
	@${CC} ${CFLAGS} ${CFLAGS_VIS} ${SRC} ${LDFLAGS} -o $@

debug: clean
	@$(MAKE) CFLAGS_VIS='${DEBUG_CFLAGS_VIS}'

clean:
	@echo cleaning
	@rm -f text

.PHONY: clean debug
