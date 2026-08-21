#!/bin/sh
# cake-cc.sh - Cake C23->C89 transpiler + gcc backend wrapper
# Usage: ./cake-cc.sh [gcc-flags] -o output source.c

set -e

CAKE="/usr/local/cake/0.14.30/cake"
GCC="${GCC:-gcc}"
TMPDIR="${TMPDIR:-/tmp}"
BAKDIR="$TMPDIR/cake-cc-$$"
mkdir -p "$BAKDIR"
trap 'rm -rf "$BAKDIR"' EXIT

OUTPUT=""
SOURCES=""
GCC_ARGS=""

for arg in "$@"; do
    case "$arg" in
        -o)
            OUTPUT_NEXT=1
            ;;
        -o*)
            OUTPUT="${arg#-o}"
            ;;
        *.c)
            SOURCES="$SOURCES $arg"
            ;;
        *)
            if [ "$OUTPUT_NEXT" = "1" ]; then
                OUTPUT="$arg"
                OUTPUT_NEXT=""
            else
                GCC_ARGS="$GCC_ARGS $arg"
            fi
            ;;
    esac
done

if [ -z "$OUTPUT" ]; then
    echo "cake-cc.sh: no output file specified (use -o)" >&2
    exit 1
fi

if [ -z "$SOURCES" ]; then
    echo "cake-cc.sh: no source files specified" >&2
    exit 1
fi

C89_FILES=""
for src in $SOURCES; do
    base=$(basename "$src" .c)
    c89="$BAKDIR/${base}_c89.c"
    "$CAKE" "$src" -o "$c89" >/dev/null || {
        echo "cake-cc.sh: cake failed for $src" >&2
        exit 1
    }
    C89_FILES="$C89_FILES $c89"
done

# shellcheck disable=SC2086
exec $GCC $GCC_ARGS -o "$OUTPUT" $C89_FILES
