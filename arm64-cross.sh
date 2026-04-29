#!/bin/sh
# Usage: arm64-cross.sh [rcc-options] file.c [file2.c ...] [-o output]
# Cross-compiles C inputs to aarch64-linux ELF binaries, tested via qemu.
#
# NOTE: This targets Linux ARM64 (ELF), not macOS ARM64 (Mach-O).
# There is no qemu-user for Darwin syscalls.  macOS ARM64 is tested
# natively on the macos-latest CI runner (Apple Silicon).
#
# Builds an ARM64-native rcc via the aarch64 cross-compiler (which defines
# __aarch64__, auto-activating the ARM64 backend), then runs it under qemu
# to produce .s files, finally assembled+linked with the cross-compiler.
#
# Environment:
#   ARM64_CC     aarch64 cross-compiler (default: aarch64-linux-gnu-gcc)
#   ARM64_QEMU   qemu user-mode runner (default: qemu-aarch64)
#   ARM64_SYSROOT sysroot for qemu -L (auto-detected from $ARM64_CC)

scriptdir="$(cd "$(dirname "$0")" && pwd)"

# --- toolchain detection ---
for cc in "${ARM64_CC:-aarch64-linux-gnu-gcc}" aarch64-redhat-linux-gcc aarch64-linux-gnu-gcc; do
    if command -v "$cc" >/dev/null 2>&1; then
        ARM64_CC="$cc"
        break
    fi
done
ARM64_QEMU="${ARM64_QEMU:-qemu-aarch64}"
ARM64_SYSROOT="${ARM64_SYSROOT:-$("$ARM64_CC" -print-sysroot 2>/dev/null)}"
# Verify sysroot is usable (contains headers/libs); fallback to known paths
if [ -z "$ARM64_SYSROOT" ] || [ ! -d "$ARM64_SYSROOT/usr/include" ]; then
    for p in /usr/aarch64-redhat-linux/sys-root/fc43 /usr/aarch64-linux-gnu/sys-root; do
        if [ -d "$p/usr/include" ]; then ARM64_SYSROOT="$p"; break; fi
    done
fi

# Build ARM64-native rcc if not already built
rcc_bin="$scriptdir/rcc-arm64"
rcc_stamp="$scriptdir/.rcc-arm64.stamp"
rcc_sources="src/main.c src/lexer.c src/preprocess.c src/parser.c src/type.c src/codegen.c src/opt.c src/alloc.c src/unicode.c src/sysinc_paths.h"
need_rebuild=0
if [ ! -x "$rcc_bin" ]; then
    need_rebuild=1
elif [ -f "$rcc_stamp" ]; then
    for src in $rcc_sources; do
        if [ "$src" -nt "$rcc_stamp" ]; then need_rebuild=1; break; fi
    done
fi
if [ $need_rebuild -eq 1 ]; then
    echo "arm64-cross.sh: building rcc-arm64 ..." >&2
    sysroot_flag=""
    [ -n "$ARM64_SYSROOT" ] && [ -d "$ARM64_SYSROOT" ] && sysroot_flag="--sysroot=$ARM64_SYSROOT"
    (cd "$scriptdir" && make -s clean 2>/dev/null; make -s CC="$ARM64_CC" CFLAGS="-std=c11 -Wall -Wextra -O2 -g $sysroot_flag" TARGET=rcc-arm64 OBJ_EXT=.arm64.o) || {
        echo "arm64-cross.sh: failed to build rcc-arm64" >&2; exit 1
    }
    cp "$scriptdir/rcc" "$rcc_bin"
    touch "$rcc_stamp"
fi

rcc_flags=""
inputs=""
output=""

while [ $# -gt 0 ]; do
    case "$1" in
    -o)
        output="$2"; shift 2 ;;
    -*)
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
        inputs="$inputs $1"; shift ;;
    *)
        inputs="$inputs $1"; shift ;;
    esac
done

inputs="${inputs# }"
if [ -z "$inputs" ]; then
    echo "arm64-cross.sh: no input files" >&2
    exit 1
fi
if [ -z "$output" ]; then
    output="a.out"
fi

s_files=""
for input in $inputs; do
    TMP_S="$(mktemp /tmp/arm64_cross_XXXXXX.s)"
    # shellcheck disable=SC2086
    if ! "$ARM64_QEMU" ${ARM64_SYSROOT:+-L "$ARM64_SYSROOT"} "$rcc_bin" $rcc_flags -S -o "$TMP_S" "$input"; then
        rm -f $s_files
        exit 1
    fi
    s_files="$s_files $TMP_S"
done

# shellcheck disable=SC2086
if [ -n "$ARM64_SYSROOT" ] && [ -d "$ARM64_SYSROOT" ]; then
    "$ARM64_CC" --sysroot="$ARM64_SYSROOT" -no-pie -o "$output" $s_files && rm -f $s_files
else
    "$ARM64_CC" -no-pie -o "$output" $s_files && rm -f $s_files
fi

ret=$?

# Optionally run under qemu
if [ $ret -eq 0 ] && [ -n "$ARM64_QEMU" ]; then
    if command -v "$ARM64_QEMU" >/dev/null 2>&1; then
        echo "=== Running under $ARM64_QEMU ==="
        if [ -n "$ARM64_SYSROOT" ] && [ -d "$ARM64_SYSROOT" ]; then
            "$ARM64_QEMU" -L "$ARM64_SYSROOT" "$output"
        else
            "$ARM64_QEMU" "$output"
        fi
        echo "=== exit: $? ==="
    fi
fi

exit $ret
