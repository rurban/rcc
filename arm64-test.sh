#!/bin/sh
# Cross-build ARM64 rcc and run tests via qemu-aarch64.
# Usage: ./arm64-test.sh [test-name]
set -e

# Locate aarch64 sysroot with headers/libs
SYSROOT="$(aarch64-linux-gnu-gcc -print-sysroot 2>/dev/null || true)"
if [ ! -f "${SYSROOT}/usr/include/stdio.h" ] && [ -d "/usr/aarch64-redhat-linux/sys-root/fc43/usr/include" ]; then
    SYSROOT="/usr/aarch64-redhat-linux/sys-root/fc43"
fi

if [ -n "${1:-}" ]; then
    make -s CC=aarch64-linux-gnu-gcc
    make -s run_tests_arm64
    export GCC_FOR_TESTS=aarch64-linux-gnu-gcc
    qemu-aarch64 ${SYSROOT:+-L "$SYSROOT"} ./run_tests_arm64 --all "$1"
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
