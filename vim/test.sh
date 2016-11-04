#!/bin/sh

[ ! -z "$CI" ] && {
	echo "Skipping tests in CI environment"
	exit 0
}

[ -z "$VIS" ] && VIS="../../vis"
[ -z "$VIM" ] && VIM="vim"

EDITORS="$VIM $VIS"

TESTS=$1
[ -z "$TESTS" ] && TESTS=$(find . -name '*.keys' | sed 's/\.keys$//g')

TESTS_RUN=0
TESTS_OK=0

$VIM --version | head -1
$VIS -v

for t in $TESTS; do
	for EDITOR in $EDITORS; do
		e=$(basename "$EDITOR");
		ERR="$t.$e.err"
		OUT="$t.$e.out"
		REF="$t.ref"
		VIM_OUT="$t.$VIM.out"
		printf "Running test %s with %s ... " "$t" "$e"
		rm -f "$OUT" "$ERR"
		[ "$e" = "$VIM" ] && EDITOR="$VIM -u NONE -U NONE -N -i NONE"
		{ cat "$t.keys"; printf "<Escape>:wq! $OUT<Enter>"; } | cpp -P 2>/dev/null | ../util/keys | $EDITOR "$t.in" 2> /dev/null
		if [ "$e" = "$VIM" ]; then
			if [ -e "$REF" ]; then
				if cmp -s "$REF" "$OUT"; then
					printf "OK\n"
				else
					printf "FAIL\n"
					diff -u "$REF" "$OUT" > "$ERR"
				fi
			elif [ -e "$VIM_OUT" ]; then
				printf "OK\n"
			else
				printf "FAIL\n"
			fi
		elif [ -e "$REF" -o -e "$VIM_OUT" ]; then
			[ -e "$VIM_OUT" ] && REF="$VIM_OUT"
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
done

printf "Tests ok %d/%d\n" $TESTS_OK $TESTS_RUN

# set exit status
[ $TESTS_OK -eq $TESTS_RUN ]
