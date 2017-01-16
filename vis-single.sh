#!/bin/sh

set -e

VISTMP="$(mktemp -d -p "${TMPDIR:-/tmp}" .vis-XXXXXX)"
trap 'rm -rf "$VISTMP"' EXIT INT QUIT TERM HUP

sed '1,/^__TAR_GZ_ARCHIVE_BELOW__$/d' "$0" | gzip -d | tar xC "$VISTMP"

PATH="$VISTMP:$PATH" "$VISTMP/vis" "$@"

exit $?

__TAR_GZ_ARCHIVE_BELOW__
