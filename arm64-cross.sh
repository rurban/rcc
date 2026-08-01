#!/bin/sh
# Usage: arm64-cross.sh [rcc-options] file.c [file2.c ...] [-o output]
# Cross-compiles C inputs to aarch64-linux ELF binaries, tested via qemu.
#
# NOTE: This targets Linux ARM64 (ELF), not macOS ARM64 (Mach-O).
# There is no qemu-user for Darwin syscalls.  macOS ARM64 is tested
# natively on the macos-latest CI runner (Apple Silicon).
#
# Runs a single rcc-arm64 invocation under qemu that compiles every
# input and links them with rcc's own native ELF linker (link_elf.c) --
# rcc-arm64 only falls back to the aarch64 cross-compiler's own linker
# internally, for inputs the native linker doesn't yet support (e.g.
# -static, -shared, unresolvable imports).  This exercises the native
# linker end to end instead of bypassing it with an external cc link
# step.  Build rcc-arm64 first:
#   make clean && make CC=aarch64-linux-gnu-gcc
#
# Environment:
#   ARM64_CC       aarch64 cross-compiler (default: aarch64-linux-gnu-gcc)
#   ARM64_QEMU     qemu user-mode runner (default: qemu-aarch64)
#   ARM64_SYSROOT  sysroot for qemu -L and (GCC-fallback) cross-compiler
#                  --sysroot

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
if [ -z "$ARM64_SYSROOT" ] || [ "$ARM64_SYSROOT" = "/" ] || { [ ! -d "$ARM64_SYSROOT/usr/include" ] && [ ! -f "$ARM64_SYSROOT/lib/ld-linux-aarch64.so.1" ]; }; then
    for p in /usr/aarch64-linux-gnu /usr/aarch64-redhat-linux/sys-root/fc43 /usr/aarch64-linux-gnu/sys-root /usr/aarch64-linux-gnu; do
        if [ -d "$p/usr/include" ] || [ -f "$p/lib/ld-linux-aarch64.so.1" ]; then ARM64_SYSROOT="$p"; break; fi
    done
fi

rcc_bin="$scriptdir/rcc-arm64"
if [ ! -x "$rcc_bin" ]; then
    echo "arm64-cross.sh: rcc-arm64 not found; build it first with:" >&2
    echo "  make clean && make CC=aarch64-linux-gnu-gcc" >&2
    exit 1
fi

rcc_flags=""
inputs=""
output=""
emit_asm=0

while [ $# -gt 0 ]; do
    case "$1" in
    -S)
        emit_asm=1; shift ;;
    -o)
        output="$2"; shift 2 ;;
    -*)
        # rcc option: collect for passing through (rcc's own CLI already
        # understands -l/-L/-static/-Wl, etc. for both its native linker
        # and the GCC fallback -- see src/main.c).
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

if [ "$emit_asm" -eq 1 ]; then
    if [ -z "$output" ]; then
        output="$(echo "$inputs" | sed 's/\.c$/.s/')"
    fi
    # shellcheck disable=SC2086
    exec "$ARM64_QEMU" ${ARM64_SYSROOT:+-L "$ARM64_SYSROOT"} "$rcc_bin" $rcc_flags -S -o "$output" $inputs
fi

if [ -z "$output" ]; then
    output="a.arm64"
fi

# Single rcc-arm64 invocation: compiles every input and links them with
# rcc's own native ELF linker.  QEMU_LD_PREFIX propagates the sysroot to
# any child process the native linker's CRT/GCC-fallback path spawns
# (compiled test binaries, or the cross-compiler on native-linker
# fallback), matching arm64-test.sh's convention.
if [ -n "$ARM64_SYSROOT" ]; then
    export QEMU_LD_PREFIX="$ARM64_SYSROOT"
fi
# shellcheck disable=SC2086
exec "$ARM64_QEMU" ${ARM64_SYSROOT:+-L "$ARM64_SYSROOT"} "$rcc_bin" $rcc_flags -o "$output" $inputs
