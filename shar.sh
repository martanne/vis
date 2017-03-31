#!/bin/sh
# Modified version of https://shiz.me/junk/code/fun/shar.sh
# The generated shell archive is automatically extracted to a
# temporary directory and the first archive member is executed.
set -e

if test $# -lt 2 ; then
	echo "usage: $0 <output> <files>"
	exit 1
fi

save() {
	for i ; do
		printf "%s\n" "$i" | sed "s/'/'\\\\''/g;1s/^/'/;\$s/\$/' \\\\/"
	done
	echo " "
}

octal_mode() {
	local mode

	mode="$(ls -ld "$1" | awk '{ print $1 }' | tr '[A-Z]' '-' | cut -c2-)"
	for i in 1 2 3 ; do
		i=0
		case "$mode" in r??*) i=$((i + 4)) ;; esac
		case "$mode" in ?w?*) i=$((i + 2)) ;; esac
		case "$mode" in ??-*) ;; ???*) i=$((i + 1)) ;; esac
		printf "%d" "$i"
		mode=$(echo "$mode" | cut -c4-)
	done
}

out=$1
tmpout=$1.tmp
exe=$2
shift

rm -f "$tmpout"
for f ; do
	if test -f "$f" ; then
		cat "$f" >> "$tmpout"
	fi
done
trap 'rm "$tmpout"' EXIT

clear=

for f ; do
	if test -z "$clear" ; then
		set --
		clear=1
	fi

	mode=$(octal_mode "$f")
	if test -f "$f" ; then
		size=$(wc -c "$f" | awk '{ print $1 }')
		echo "adding: $f (mode $mode, $size bytes)"
		set -- "$@" "f+$mode+$size:$f"
	elif test -d "$f" ; then
		echo "adding directory: $f (mode $mode)"
		set -- "$@" "d+$mode:$f"
	else
		echo "can't add unknown file type $f" >&2
	fi
done

cat >"$out"<<HEADER
#!/bin/sh
set -e

outdir="\$(mktemp -d -p "\${TMPDIR:-/tmp}" .unshar-XXXXXX)"
if test \$? -ne 0 -o ! -d "\$outdir" ; then
	echo "failed to create temporary directory"
	exit 1
fi
trap 'rm -rf "\$outdir"' EXIT INT QUIT TERM HUP
argc=\$#

self="\$0"
set -- $(save "$@") "\$@"

exec 3<"\$self"
while read -r l ; do
	test "\$l" = "exit 0;" && break
done <&3

while test \$# -gt \$argc ; do
	file="\$1"
	shift
	meta="\${file%%:*}"
	fn="\${file#*:}"
	type="\${meta%%+*}"
	meta="\${meta#*+}"
	mode="\${meta%%+*}"
	size="\${meta#*+}"
	base="\${fn%/*}"

	case "\$type" in
	f)
		echo "unpacking: \$fn (mode \$mode, \$size bytes)"
		test "\$base" != "\$fn" && mkdir -p "\$outdir/\$base"
		dd of="\$outdir/\$fn" bs="\$size" count=1 >/dev/null <&3
		;;
	d)
		echo "creating: \$fn (mode \$mode)"
		mkdir -p "\$outdir/\$fn"
		;;
	esac
	chmod "\$mode" "\$outdir/\$fn" || :
done >/dev/null 2>&1

PATH="\$outdir:\$PATH" "\$outdir/$exe" "\$@"
exit \$?

exit 0;
HEADER

cat "$tmpout" >> "$out"
chmod +x "$out"

