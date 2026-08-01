#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Usage: mingw-cross.sh [rcc-options] file.c [file2.c ...] [-o output.exe]
# Wraps rcc.exe (Windows PE) under Wine for cross-compilation.
#
# Passes all non -o options through to a single rcc.exe invocation that
# compiles every input and links them with rcc's own native PE linker
# (link_pe.c) -- rcc.exe only falls back to x86_64-w64-mingw32-gcc's own
# linker internally, for inputs the native linker doesn't yet support
# (e.g. -static, -shared, unresolvable imports).  This exercises the
# native linker end to end instead of bypassing it with an external
# gcc link step.

scriptdir="$(cd "$(dirname "$0")" && pwd)"

rcc_flags=""
inputs=""
output=""
emit_asm=0

while [ $# -gt 0 ]; do
	case "$1" in
	-S)
		emit_asm=1
		shift
		;;
	-o)
		output="$2"
		shift 2
		;;
	-*)
		# rcc option: collect for passing through (rcc's own CLI already
		# understands -l/-L/-pthread/-static/-Wl, etc. for both its
		# native linker and the GCC fallback -- see src/main.c).
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

WINEDEBUG=fixme-all
WINEDLLOVERRIDES="winedbg=d"
WINENOPOPUPS=1
export WINEDEBUG WINEDLLOVERRIDES WINENOPOPUPS

if [ "$emit_asm" -eq 1 ]; then
	if [ -z "$output" ]; then
		output="$(echo "$inputs" | sed 's/\.c$/.s/')"
	fi
	# shellcheck disable=SC2086
	exec wine "$scriptdir/rcc.exe" $rcc_flags -S -o "$output" $inputs
fi

if [ -z "$output" ]; then
	output="a.exe"
fi

# Ensure libwinpthread-1.dll is available for Wine (needed by mingw-w64 CRT)
if [ ! -f "$HOME/.wine/drive_c/windows/system32/libwinpthread-1.dll" ]; then
    cp /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll \
       "$HOME/.wine/drive_c/windows/system32/" 2>/dev/null || true
fi

# Ensure libmingw helper object is built (needed for atexit/on_exit etc.).
# NOT passed as an explicit input: rcc.exe already auto-includes
# lib/rcc_mingw.obj on its GCC-fallback path when it exists relative to
# cwd (see src/main.c) -- passing it here too would double-link it.
if [ ! -f "$scriptdir/lib/rcc_mingw.obj" ]; then
    x86_64-w64-mingw32-gcc -c "$scriptdir/lib/rcc_mingw.c" -o "$scriptdir/lib/rcc_mingw.obj" || exit 1
fi

# Single rcc.exe invocation: compiles every input and links them with
# rcc's own native PE linker.
#
# rcc.exe is always asked to write a .exe-suffixed temp path, then that
# is moved to the caller's real $output.  Two independent Windows-side
# quirks force this indirection:
#  1. rcc.exe's own chmod(0755) on the output (see link_pe.c) goes
#     through Wine's msvcrt _chmod shim, which only toggles the DOS
#     read-only attribute -- it never sets a real Unix execute bit on
#     the host file.
#  2. When the native linker can't handle an input, rcc.exe falls back
#     to the real Windows gcc.exe under Wine; gcc.exe's own linker
#     driver silently APPENDS ".exe" to any -o path lacking a
#     recognized extension (linking .o inputs, unlike gcc on Linux,
#     which honors the exact name given) -- so an extensionless
#     $output would link successfully but leave nothing at $output.
# Requesting a .exe name up front avoids both: the native linker
# honors it exactly, and gcc.exe's own extension no longer differs
# from what we asked for.  Move to $output and chmod +x from the
# POSIX side so access(X_OK) callers (e.g. run_tests) and binfmt_misc
# see an executable file regardless of which linker produced it.
tmp_exe="$(mktemp -u "${TMPDIR:-/tmp}/mingw_cross_XXXXXX.exe")"
# shellcheck disable=SC2086
wine "$scriptdir/rcc.exe" $rcc_flags -o "$tmp_exe" $inputs
status=$?
if [ $status -eq 0 ] && [ -f "$tmp_exe" ]; then
	mv -f "$tmp_exe" "$output" && chmod +x "$output"
	status=$?
else
	rm -f "$tmp_exe"
fi
exit $status
