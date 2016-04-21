#!/bin/bash

[ -z "$VIS" ] && VIS="../../vis"
$VIS -v

TESTS_OK=0
TESTS_RUN=0

export VIS_PATH=.
export VIS_THEME=theme

if [ $# -gt 0 ]; then
	test_files=$@
else
	test_files="$(find . -type f -name "*.in") basic_empty_file.in"
fi

for t in $test_files; do
	TESTS_RUN=$((TESTS_RUN + 1))
	t=${t%.in}
	t=${t#./}
	$VIS $t.in

	printf "%-30s" "$t"
	if [ -e $t.out ]; then
		if cmp -s $t.ref $t.out 2> /dev/null; then
			printf "PASS\n"
			TESTS_OK=$((TESTS_OK + 1))
		else
			printf "FAIL\n"
			diff -u $t.ref $t.out > $t.err
		fi
	elif [ -e $t.status ]; then
		if ! grep -v true $t.status > /dev/null; then
			printf "PASS\n"
			TESTS_OK=$((TESTS_OK + 1))
		else
			printf "FAIL\n"
			printf "$t\n" > $t.err
			grep -vn true $t.status >> $t.err
		fi
	else
		printf "FAIL\n"
	fi
done

printf "Tests ok %d/%d\n" $TESTS_OK $TESTS_RUN

# set exit status
[ $TESTS_OK -eq $TESTS_RUN ]
