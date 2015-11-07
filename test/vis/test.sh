#!/bin/sh

[ -z "$VIS" ] && VIS="../../vis"

TESTS=$1
[ -z "$TESTS" ] && TESTS=$(find . -name '*.in' | sed 's/\.in$//g')

TESTS_RUN=0
TESTS_OK=0

$VIS -v

for t in $TESTS; do
	ERR="$t.err"
	OUT="$t.out"
	REF="$t.ref"
	printf "Running test %s ... " "$t"
	rm -f "$OUT" "$ERR"
	{ cat "$t.keys"; printf "<Escape>:wq! $OUT<Enter>"; } | cpp -P | ../util/keys | $VIS "$t.in" 2> /dev/null
	if [ -e "$OUT" ]; then
		if cmp -s "$REF" "$OUT"; then
			printf "OK\n"
			TESTS_OK=$((TESTS_OK+1))
		else
			printf "FAIL\n"
			diff -u "$REF" "$OUT" > "$ERR"
		fi
		TESTS_RUN=$((TESTS_RUN+1))
	fi
done

printf "Tests ok %d/%d\n" $TESTS_OK $TESTS_RUN

# set exit status
[ $TESTS_OK -eq $TESTS_RUN ]
