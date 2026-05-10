#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Usage: mingw-cross.sh [rcc-options] file.c [file2.c ...] [-o output.exe]
# Wraps rcc.exe (Windows PE) under Wine for cross-compilation.
#
# Passes all non -o options through to rcc's -S invocation, then links
# the resulting .s files with x86_64-w64-mingw32-gcc.

scriptdir="$(cd "$(dirname "$0")" && pwd)"

rcc_flags=""
ldflags=""
inputs=""
output=""

while [ $# -gt 0 ]; do
	case "$1" in
	-o)
		output="$2"
		shift 2
		;;
        -l*|-L*|-pthread)
		ldflags="$ldflags $1"
		shift
		;;
	-*)
		# rcc option: collect for passing through
		rcc_flags="$rcc_flags $1"
		if [ $# -gt 1 ]; then
			case "$1" in
			-o|-I|-L|-D|-U) rcc_flags="$rcc_flags $2"; shift 2 ;;
			*) shift ;;
			esac
		else
			shift
		fi
		;;
	*.c|*.s)
		inputs="$inputs $1"
		shift
		;;
	*)
		# Assume it's an input file
		inputs="$inputs $1"
		shift
		;;
	esac
done

inputs="${inputs# }"

if [ -z "$inputs" ]; then
	echo "mingw-cross.sh: no input files" >&2
	exit 1
fi

if [ -z "$output" ]; then
	output="a.exe"
fi

WINEDEBUG=fixme-all
WINEDLLOVERRIDES="winedbg=d"
WINENOPOPUPS=1
export WINEDEBUG WINEDLLOVERRIDES WINENOPOPUPS

# Ensure libwinpthread-1.dll is available for Wine (needed by mingw-w64 CRT)
if [ ! -f "$HOME/.wine/drive_c/windows/system32/libwinpthread-1.dll" ]; then
    cp /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll \
       "$HOME/.wine/drive_c/windows/system32/" 2>/dev/null || true
fi

# Ensure libmingw helper object is built (needed for atexit/on_exit etc.)
if [ ! -f "$scriptdir/lib/mingw.obj" ]; then
    x86_64-w64-mingw32-gcc -c "$scriptdir/lib/mingw.c" -o "$scriptdir/lib/mingw.obj" || exit 1
fi

o_files=""
for input in $inputs; do
	TMP_O="$(mktemp -u /tmp/mingw_cross_XXXXXX.o)"
	# shellcheck disable=SC2086
	if ! wine "$scriptdir/rcc.exe" $rcc_flags -c -o "$TMP_O" "$input" $ldflags; then
		rm -f $o_files
		exit 1
	fi
	o_files="$o_files $TMP_O"
done

# shellcheck disable=SC2086
x86_64-w64-mingw32-gcc -o "$output" $o_files "$scriptdir/lib/mingw.obj" $ldflags && rm -f $o_files
