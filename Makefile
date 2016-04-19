test:
	@$(MAKE) -C core
	@$(MAKE) -C vim
	@$(MAKE) -C vis

test-local: keys-local test

keys-local:
	@$(MAKE) -C util $@

clean:
	@$(MAKE) -C core clean
	@$(MAKE) -C vim clean
	@$(MAKE) -C vis clean

.PHONY: test test-local keys-local clean
