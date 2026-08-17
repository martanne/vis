-include ../../config.mk

keys: keys.c
	@echo Compiling keys utility
	$(CC) $(CFLAGS) -I../.. keys.c $(LDFLAGS) $(LDFLAGS_CURSES) -Wno-unused-function -o keys

clean:
	@echo cleaning
	@rm -f keys

.PHONY: clean
