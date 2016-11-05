#!/bin/sh

NL='
'
PLAN9="/usr/local/plan9/bin"

[ -z "$VIS" ] && VIS="../../vis"
[ -z "$SSAM" ] && SSAM="$PLAN9/ssam"

type "$SSAM" >/dev/null 2>&1 || {
	echo "ssam(1) not found, skipping tests"
	exit 0
}

TESTS=$1
[ -z "$TESTS" ] && TESTS=$(find . -name '*.cmd' | sed 's/\.cmd$//g')

TESTS_RUN=0
TESTS_OK=0

$VIS -v

for t in $TESTS; do
	IN="$t.in"
	CMD="$(cat $t.cmd)"
	SAM_OUT="$t.sam.out"
	SAM_ERR="$t.sam.err"
	VIS_OUT="$t.vis.out"
	VIS_ERR="$t.vis.err"
	REF="$t.ref"
	rm -f "$SAM_OUT" "$SAM_ERR" "$VIS_OUT" "$VIS_ERR"
	printf "Running test %s with sam ... " "$t"
	cat "$IN" | PATH="$PLAN9:$PATH" $SSAM "$CMD" > "$SAM_OUT"
	if [ $? -ne 0 ]; then
		printf "ERROR\n"
	elif [ -e "$REF" ]; then
		if cmp -s "$REF" "$SAM_OUT"; then
			printf "OK\n"
		else
			printf "FAIL\n"
			diff -u "$REF" "$SAM_OUT" > "$SAM_ERR"
		fi
	elif [ -e "$SAM_OUT" ]; then
		REF="$SAM_OUT"
		printf "OK\n"
	fi

	if [ ! -e "$REF" ]; then
		printf " No reference solution, skipping.\n"
		continue
	fi

	TESTS_RUN=$((TESTS_RUN+1))

	printf "Running test %s with vis ... " "$t"

	$VIS "+{ $NL $CMD $NL wq! $VIS_OUT $NL }" "$IN" 2>/dev/null
	if [ $? -ne 0 -o ! -e "$VIS_OUT" ]; then
		printf "ERROR\n"
	elif cmp -s "$REF" "$VIS_OUT"; then
		printf "OK\n"
		TESTS_OK=$((TESTS_OK+1))
	else
		printf "FAIL\n"
		diff -u "$REF" "$VIS_OUT" > "$VIS_ERR"
	fi
done

printf "Tests ok %d/%d\n" $TESTS_OK $TESTS_RUN

# set exit status
[ $TESTS_OK -eq $TESTS_RUN ]
