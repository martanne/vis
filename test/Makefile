test:
	@$(MAKE) -C text
	@$(MAKE) -C vim
	@$(MAKE) -C vis

test-local: keys-local test

keys-local:
	@$(MAKE) -C util $@

clean:
	@$(MAKE) -C text clean
	@$(MAKE) -C vim clean
	@$(MAKE) -C vis clean

.PHONY: test test-local keys-local clean
