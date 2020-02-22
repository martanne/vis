#!/bin/sh

[ -z "$VIS" ] && VIS="../../vis"
[ -z "$VIM" ] && VIM="vim"

TESTS=$1
[ -z "$TESTS" ] && TESTS=$(find . -name '*.keys' | sed 's/\.keys$//g')

TESTS_RUN=0
TESTS_OK=0
TESTS_SKIP=0

export VIS_PATH=.

if type "$VIM" >/dev/null 2>&1; then
	EDITORS="$VIM $VIS"
	$VIM --version | head -1
else
	EDITORS="$VIS"
fi

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
		{ cat "$t.keys"; printf "<Escape>:w! $OUT<Enter>:qall!<Enter>\n"; } | cpp -P 2>/dev/null | sed 's/[ \t]*$//' | ../util/keys | $EDITOR "$t.in" >/dev/null 2>&1
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
		else
			TESTS_SKIP=$((TESTS_SKIP+1))
			printf "SKIPPED\n"
		fi
	done
done

printf "Tests ok %d/%d, skipped %d\n" $TESTS_OK $TESTS_RUN $TESTS_SKIP

# set exit status
[ $TESTS_OK -eq $TESTS_RUN ]
