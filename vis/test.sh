#!/bin/sh

export VIS_PATH=.
[ -z "$VIS" ] && VIS="../../vis"
$VIS -v

TESTS_OK=0
TESTS_RUN=0

if [ $# -gt 0 ]; then
	test_files=$*
else
	printf ':help\n:/ Lua paths/,$ w help\n:qall\n' | $VIS 2> /dev/null && cat help && rm -f help
	test_files="$(find . -type f -name '*.in')"
fi

for t in $test_files; do
	TESTS_RUN=$((TESTS_RUN + 1))
	t=${t%.in}
	t=${t#./}
	printf "%-50s" "$t"

	$VIS "$t".in < /dev/null > /dev/null 2>&1

	if [ -e "$t".out ]; then
		if cmp -s "$t".ref "$t".out 2> /dev/null; then
			printf "PASS\n"
			TESTS_OK=$((TESTS_OK + 1))
		else
			printf "FAIL\n"
			diff -u "$t".ref "$t".out > "$t".err
		fi
	else
		printf "ERROR\n"
	fi
done

printf "Tests ok %d/%d\n" $TESTS_OK $TESTS_RUN

# set exit status
[ $TESTS_OK -eq $TESTS_RUN ]
