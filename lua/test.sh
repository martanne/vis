#!/bin/bash

export VIS_PATH=.
export VIS_THEME=theme
printf "<Escape>Q:q<Enter>" | ../util/keys | vis

TESTS_OK=0
TESTS_RUN=0

ref_files=$(find . -type f -name "*.ref")

for ref in $ref_files; do
	TESTS_RUN=$((TESTS_RUN + 1))
	out=${ref%.ref}.out
	printf "%-30s" "$ref"
	if cmp $ref $out 2> /dev/null; then
		printf "PASS\n"
		TESTS_OK=$((TESTS_OK + 1))
	else
		printf "FAIL\n"
		diff -u $ref $out
	fi
done

true_files=$(find . -type f -name "*.true")

for t in $true_files; do
	TESTS_RUN=$((TESTS_RUN + 1))
	printf "%-30s" "$t"
	if ! grep -v true $t > /dev/null; then
		printf "PASS\n"
		TESTS_OK=$((TESTS_OK + 1))
	else
		printf "FAIL\n"
		grep -vn true $t
	fi
done

printf "Tests ok %d/%d\n" $TESTS_OK $TESTS_RUN

# set exit status
[ $TESTS_OK -eq $TESTS_RUN ]
