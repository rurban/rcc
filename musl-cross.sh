#!/bin/sh
# Usage: musl-cross.sh [rcc-options] file.c [file2.c ...] [-o output]
# Cross-compiles C inputs against musl libc, statically linked.
#
# Compiles using the system rcc (x86-64) with musl headers and links
# against musl's static libc.  This exercises rcc's ability to compile
# code targeting a different C library (musl) while using the host's
# x86-64 codegen and native ELF linker.
#
# Environment:
#   MUSL_CC        musl-gcc wrapper (default: musl-gcc)
#   MUSL_SYSROOT   sysroot (default: /usr/x86_64-linux-musl) on proper systems
# paths:
#   MUSL_INCLUDE   /usr/x86_64-linux-musl/include on proper systems, /usr/include/x86_64-linux-musl on debian
#   MUSL_LIB       /usr/x86_64-linux-musl/lib on proper systems, /usr/lib/x86_64-linux-musl on debian

scriptdir="$(cd "$(dirname "$0")" && pwd)"
MUSL_CC="${MUSL_CC:-musl-gcc}"
MUSL_SYSROOT="${MUSL_SYSROOT:-/usr/x86_64-linux-musl}"
MUSL_INCLUDE="$MUSL_SYSROOT/include"
MUSL_LIB="$MUSL_SYSROOT/lib64"

if [ ! -d "$MUSL_SYSROOT/include" ]; then
    if [ ! -d "/usr/include/x86_64-linux-musl" ]; then
        echo "musl sysroot not found at $MUSL_SYSROOT" >&2
        echo "Install: dnf install musl-devel musl-libc-static" >&2
        echo "or: apt install musl musl-dev" >&2
        exit 1
    else
        MUSL_INCLUDE="/usr/include/x86_64-linux-musl"
        MUSL_LIB="/usr/lib/x86_64-linux-musl"
    fi
fi

# The host rcc is a glibc build; its <features.h> chain would otherwise
# define __GLIBC__/__GLIBC_MINOR__/__GLIBC_PREREQ for musl-targeted TUs.
# Undef them explicitly and define __MUSL__ so target code sees the musl
# libc, matching a real musl-gcc build.
MUSL_LIBC_FLAGS="-U__GLIBC__ -U__GLIBC_MINOR__ -U__GLIBC_PREREQ -D__MUSL__"

rcc_bin="$scriptdir/rcc"
if [ ! -x "$rcc_bin" ]; then
    echo "rcc not found at $rcc_bin — build it first" >&2
    exit 1
fi

rcc_flags=""
inputs=""
output=""
emit_asm=0

compile_only=0
while [ $# -gt 0 ]; do
    case "$1" in
        -S) emit_asm=1; rcc_flags="$rcc_flags $1" ;;
        -c) compile_only=1; rcc_flags="$rcc_flags $1" ;;
        -o) output="$2"; shift ;;
        -o*) output="${1#-o}" ;;
        # Two-argument options: keep the value glued so rcc sees them as
        # one token. run_tests passes "-I ." / "-isystem dir" as separate
        # argv elements; without this the value would fall through to the
        # positional-input list and be treated as a source file.
        -I|-L|-isystem|-idirafter|-include|-D|-U|-Wl|-Xlinker)
            rcc_flags="$rcc_flags $1$2"; shift ;;
        -I*|-L*|-isystem*|-idirafter*|-include*|-D*|-U*|-Wl*)
            rcc_flags="$rcc_flags $1" ;;
        -*)  rcc_flags="$rcc_flags $1" ;;
        *)   inputs="$inputs $1" ;;
    esac
    shift
done

inputs="${inputs# }"
if [ -z "$inputs" ]; then
    echo "Usage: musl-cross.sh [rcc-options] file.c ..." >&2
    exit 1
fi

if [ "$emit_asm" -eq 1 ]; then
    if [ -z "$output" ]; then output="a.s"; fi
    GCC_INCLUDE=$("$MUSL_CC" -print-search-dirs 2>/dev/null | grep "^install:" | sed 's/^install: //')include
    # shellcheck disable=SC2086
    exec "$rcc_bin" $rcc_flags $MUSL_LIBC_FLAGS -nostdinc \
        -isystem "$GCC_INCLUDE" \
        -isystem "$MUSL_INCLUDE" \
        -o "$output" $inputs
fi

if [ -z "$output" ]; then
    output="a.musl"
fi

# Single rcc invocation: compile with musl headers, link statically
# against musl's libc using rcc's native ELF linker.
# -nostdinc drops system headers; -isystem adds back GCC's intrinsic
# headers (xmmintrin.h etc.) and musl's own headers.
GCC_INCLUDE=$("$MUSL_CC" -print-search-dirs 2>/dev/null | grep "^install:" | sed 's/^install: //')include
# shellcheck disable=SC2086
if [ "$compile_only" -eq 1 ]; then
    # -c: emit an object only, no link step (no -L/-static, no chmod).
    "$rcc_bin" $rcc_flags $MUSL_LIBC_FLAGS -nostdinc \
        -isystem "$GCC_INCLUDE" \
        -isystem "$MUSL_INCLUDE" \
        -o "$output" $inputs
else
    "$rcc_bin" $rcc_flags $MUSL_LIBC_FLAGS -nostdinc \
        -isystem "$GCC_INCLUDE" \
        -isystem "$MUSL_INCLUDE" \
        -o "$output" $inputs \
        -L"$MUSL_LIB" -static
    status=$?
    if [ $status -eq 0 ] && [ -f "$output" ]; then
        chmod +x "$output"
    fi
    exit $status
fi
