test:
	@$(MAKE) -C core
	@$(MAKE) -C lua
	@if [ -z "$$CI" -o "$$TRAVIS_OS_NAME" = "linux" ]; then $(MAKE) -C vis; else echo "Skipping vis tests"; fi
	@$(MAKE) -C sam
	@if [ -z "$$CI" ]; then $(MAKE) -C vim; else echo "Skipping vim tests"; fi

clean:
	@$(MAKE) -C core clean
	@$(MAKE) -C lua clean
	@$(MAKE) -C vis clean
	@$(MAKE) -C sam clean
	@$(MAKE) -C vim clean

.PHONY: test clean
