-include ../../config.mk

TERMKEY_OBJ = $(TERMKEY_SRC:%.c=../../obj/%.o)

keys: keys.c $(TERMKEY_OBJ)
	@echo Compiling keys utility
	$(CC) $(CFLAGS) $(CFLAGS_TERMKEY) keys.c $(TERMKEY_OBJ) $(LDFLAGS) $(LDFLAGS_TERMKEY) $(LDFLAGS_CURSES) -o keys

clean:
	@echo cleaning
	@rm -f keys

.PHONY: clean
