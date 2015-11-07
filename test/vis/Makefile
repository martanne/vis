test: ../../vis ../util/keys
	@./test.sh

../../vis: ../../*.[ch]
	@echo Compiling vis
	@$(MAKE) -C ../..

../util/keys: ../util/keys.c
	@$(MAKE) -C ../util

clean:
	@echo cleaning
	@find . -name '*.out' -o -name '*.err' | xargs rm -f

.PHONY: clean test
