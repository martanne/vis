.PHONY=all clean test

all: clean test

clean:
	rm -f *.out *.err

test:
	@./test.sh

%: %.in
	@./test.sh $@
