#!/bin/sh
# Cross-build ARM64 rcc and run the TCC test suite against it.
set -e
trap 'rm -f src/sysinc_paths.h src/gcc_predefined.h' EXIT

# Locate aarch64 sysroot with headers/libs
SYSROOT="$(aarch64-linux-gnu-gcc -print-sysroot 2>/dev/null || true)"
if [ ! -f "${SYSROOT}/usr/include/stdio.h" ] && [ -d "/usr/aarch64-redhat-linux/sys-root/fc43/usr/include" ]; then
    SYSROOT="/usr/aarch64-redhat-linux/sys-root/fc43"
fi

make leanclean
make -s CC=aarch64-linux-gnu-gcc
make -s run_tests_arm64
export GCC_FOR_TESTS=aarch64-linux-gnu-gcc
timeout 3m qemu-aarch64 ${SYSROOT:+-L "$SYSROOT"} ./run_tests_arm64 --tcc --no-color
