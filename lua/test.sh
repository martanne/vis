#!/bin/bash

TESTS_OK=0
TESTS_RUN=0

export VIS_PATH=.
export VIS_THEME=theme

if [ $# -gt 0 ]; then
	test_files=$@
else
	test_files=$(find . -type f -name "*.in")
fi

for t in $test_files; do
	TESTS_RUN=$((TESTS_RUN + 1))
	t=${t%.in}
	t=${t#./}
# 	vis $t.in
	printf "<Escape>Q:q<Enter>" | ../util/keys | vis $t.in

	printf "%-30s" "$t"
	if [ -e $t.out ]; then
		if cmp $t.ref $t.out 2> /dev/null; then
			printf "PASS\n"
			TESTS_OK=$((TESTS_OK + 1))
		else
			printf "FAIL\n"
			diff -u $t.ref $t.out
		fi
	elif [ -e $t.true ]; then
		if ! grep -v true $t.true > /dev/null; then
			printf "PASS\n"
			TESTS_OK=$((TESTS_OK + 1))
		else
			printf "FAIL\n"
			grep -vn true $t.true
		fi
	else
		printf "FAIL\n"
	fi
done

printf "Tests ok %d/%d\n" $TESTS_OK $TESTS_RUN

# set exit status
[ $TESTS_OK -eq $TESTS_RUN ]
