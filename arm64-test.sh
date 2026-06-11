#!/bin/sh
# Cross-build ARM64 rcc and run tests via qemu-aarch64.
# Usage: ./arm64-test.sh [test-name]
set -e

# Locate aarch64 sysroot with the dynamic loader for qemu -L
SYSROOT="$(aarch64-linux-gnu-gcc -print-sysroot 2>/dev/null || true)"
if [ ! -f "${SYSROOT}/lib/ld-linux-aarch64.so.1" ]; then
    SYSROOT=""
    for d in /usr/aarch64-linux-gnu /usr/aarch64-redhat-linux/sys-root/fc43; do
        if [ -f "$d/lib/ld-linux-aarch64.so.1" ]; then
            SYSROOT="$d"
            break
        fi
    done
fi
# Children (rcc-arm64, compiled tests) run via binfmt_misc qemu, which does
# not inherit -L; QEMU_LD_PREFIX covers both.
if [ -n "$SYSROOT" ]; then
    export QEMU_LD_PREFIX="$SYSROOT"
fi

if [ -n "${1:-}" ]; then
    make -s CC=aarch64-linux-gnu-gcc
    make -s run_tests_arm64
    export GCC_FOR_TESTS=aarch64-linux-gnu-gcc
    qemu-aarch64 ${SYSROOT:+-L "$SYSROOT"} ./run_tests_arm64 "$1"
else
    make leanclean
    make -s CC=aarch64-linux-gnu-gcc
    make -s run_tests_arm64
    trap 'make leanclean; make -s' EXIT
    echo "==> Running full test suite via run_tests_arm64 under qemu..."
    echo ""
    export GCC_FOR_TESTS=aarch64-linux-gnu-gcc
    qemu-aarch64 ${SYSROOT:+-L "$SYSROOT"} ./run_tests_arm64 --all
fi
