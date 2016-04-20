test:
	@$(MAKE) -C core
	@$(MAKE) -C vim
	@$(MAKE) -C vis
	@$(MAKE) -C lua

test-local: keys-local test

keys-local:
	@$(MAKE) -C util $@

clean:
	@$(MAKE) -C core clean
	@$(MAKE) -C vim clean
	@$(MAKE) -C vis clean
	@$(MAKE) -C lua clean

.PHONY: test test-local keys-local clean
