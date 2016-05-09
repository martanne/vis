-include ../../config.mk

keys: keys.c
	@echo Compiling keys utility
	$(CC) $(CFLAGS) $(CFLAGS_TERMKEY) keys.c $(LDFLAGS) $(LDFLAGS_TERMKEY) $(LDFLAGS_CURSES) -o keys

clean:
	@echo cleaning
	@rm -f keys

.PHONY: clean
