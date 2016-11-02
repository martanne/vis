test: ../../vis
	@./test.sh

../../vis: ../../*.[ch]
	@echo Compiling vis
	@$(MAKE) -C ../..

clean:
	@echo cleaning
	@find . -name '*.out' -o -name '*.err' | xargs rm -f

.PHONY: clean test
