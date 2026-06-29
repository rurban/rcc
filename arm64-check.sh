#!/bin/sh
# Cross-build ARM64 rcc and run the TCC test suite against it.
set -e
trap 'rm -f src/sysinc_paths.h src/gcc_predefined.h' EXIT

# Locate aarch64 sysroot with headers/libs. -print-sysroot may return "/"
# (Debian/Ubuntu cross gcc), which lacks the aarch64 loader; probe the
# multiarch locations for ld-linux-aarch64.so.1 instead.
SYSROOT="$(aarch64-linux-gnu-gcc -print-sysroot 2>/dev/null || true)"
if [ ! -f "${SYSROOT}/lib/ld-linux-aarch64.so.1" ]; then
    for d in /usr/aarch64-linux-gnu /usr/aarch64-redhat-linux/sys-root/fc43; do
        if [ -f "$d/lib/ld-linux-aarch64.so.1" ]; then
            SYSROOT="$d"
            break
        fi
    done
fi
# Test binaries are exec'd directly (binfmt_misc); qemu needs the prefix
# from the environment there, -L only covers the run_tests process itself.
export QEMU_LD_PREFIX="$SYSROOT"

# Host wrappers such as stdbuf set LD_PRELOAD to x86_64 libraries; qemu then
# propagates that into ARM64 test binaries and their output no longer matches.
unset LD_PRELOAD

make leanclean
make -s CC=aarch64-linux-gnu-gcc
make -s run_tests_arm64
ln -sf rcc-arm64 rcc
export GCC_FOR_TESTS=aarch64-linux-gnu-gcc
timeout 10m qemu-aarch64 ${SYSROOT:+-L "$SYSROOT"} ./run_tests_arm64 --tcc --no-color
