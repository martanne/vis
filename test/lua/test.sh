#!/bin/sh

export VIS_PATH=.
[ -z "$VIS" ] && VIS="../../vis"

# run vis under gdb
debug_run() {
	# $1 is $VIS, $2 is "$t.in"
	gdb --batch -ex "run" -ex "bt" --args "$1" "$2"
}

DEBUG_MODE=0
if [ "$1" = "-d" ] || [ "$1" = "--debug" ]; then
	DEBUG_MODE=1
	shift
fi

$VIS -v

if ! $VIS -v | grep '+lua' >/dev/null 2>&1; then
	echo "vis compiled without lua support, skipping tests"
	exit 0
fi

TESTS_OK=0
TESTS_RUN=0

if [ $# -gt 0 ]; then
	test_files=$*
else
	test_files="$(find . -type f -name '*.lua' -a ! -name visrc.lua)"
fi

for t in $test_files; do
	TESTS_RUN=$((TESTS_RUN + 1))
	t=${t%.lua}
	t=${t#./}

	if [ $DEBUG_MODE -eq 1 ]; then
		printf "Debugging %s\n" "$t"
		debug_run "$VIS" "$t.in" 2> "$t.err"
	else
		printf "% -30s" "$t"

		if "$VIS" "$t.in" < /dev/null 2> "$t.err" > "$t.out"; then
			TESTS_OK=$((TESTS_OK + 1))
			printf "OK\n"
		else
			printf "FAIL\n"
			cat "$t.out"
			cat "$t.err"
		fi
	fi
done

if [ $DEBUG_MODE -eq 1 ]; then
	exit 0
fi

printf "Tests ok %d/%d\n" "$TESTS_OK" "$TESTS_RUN"

# set exit status
[ $TESTS_OK -eq $TESTS_RUN ]
