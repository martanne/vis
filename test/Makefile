test:
	@$(MAKE) -C core
	@$(MAKE) -C lua
	@$(MAKE) -C vis
	@$(MAKE) -C sam
	@$(MAKE) -C vim

clean:
	@$(MAKE) -C core clean
	@$(MAKE) -C lua clean
	@$(MAKE) -C vis clean
	@$(MAKE) -C sam clean
	@$(MAKE) -C vim clean
	@$(MAKE) -C util clean

.PHONY: test clean
