all: clean ../../vis test

../../vis: ../../*.[ch]
	@echo Compiling vis
	@$(MAKE) -C ../..

clean:
	@rm -f *.out *.err *.busted

test:
	@./test.sh

%: %.in
	@./test.sh $@

.PHONY: all clean test
