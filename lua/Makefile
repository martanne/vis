.PHONY=all clean test

all: clean test

clean:
	rm -f *.out *.true

test:
	@./test.sh

%: %.in
	@./test.sh $@
