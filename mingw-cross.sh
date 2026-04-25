#!/bin/sh
# Usage: mingw-cross.sh file.c [-o output.exe]
scriptdir="$(cd "$(dirname "$0")" && pwd)"
input="$1"
base="${input%.c}"
shift

# Parse -o option
output=""
while [ $# -gt 0 ]; do
	case "$1" in
	-o)
		output="$2"
		shift 2
		;;
	*) shift ;;
	esac
done

if [ -z "$output" ]; then
	output="$(basename "$base.exe")"
fi
# Use a temp directory for the intermediate .s file to avoid Wine path issues
TMP_S="$(mktemp -u /tmp/mingw_cross_XXXXXX.s)"

WINEDEBUG=fixme-all
WINEDLLOVERRIDES="winedbg=d"
export WINEDEBUG WINEDLLOVERRIDES

wine "$scriptdir/rcc.exe" -S "$input" -o "$TMP_S" &&
	x86_64-w64-mingw32-gcc "$TMP_S" -o "$output" && rm "$TMP_S"
