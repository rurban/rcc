#!/bin/sh
# Build rcc-darwin and run the TCC test suite for arm64-darwin.
# Tests compile+link only (can't execute Mach-O on Linux).
# Pre-requisites: tools/install-osxcross.sh
# Usage: ./darwin-test.sh [test-name]
set -e

trap 'make -s clean; make -s' EXIT

echo "==> Building rcc-darwin (host binary, Darwin-targeted codegen)..."
make -s clean
make -s MACHINE=arm64-apple-darwin23 CC=gcc CFLAGS="-std=c11 -Wall -Wextra -O2 -g -fPIC -D__APPLE__ -DARCH_ARM64" \
    TARGET=rcc-darwin OBJ_EXT=.darwin.o

if [ -n "${1:-}" ]; then
    # Run a single test directly (compile+link only)
    TEST_BASE="$1"
    TEST_SRC="tinycc/tests/tests2/${TEST_BASE}.c"
    TMP_EXE="/tmp/rcc_test_${TEST_BASE}_$$"
    printf "  %-40s " "${TEST_BASE}..."
    if ./darwin-cross.sh "$TEST_SRC" -o "$TMP_EXE" 2>/dev/null; then
        printf "PASS (compile OK)\n"
    else
        printf "COMPILE FAIL\n"
        exit 1
    fi
    rm -f "$TMP_EXE"
else
    echo "==> Running full test suite sequentially via darwin-cross.sh..."
    echo ""
    ./run_tests ./darwin-cross.sh --all
fi
