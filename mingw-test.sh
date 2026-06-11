#!/bin/sh
# Cross-build Windows rcc and run the tests against it.
# Usage: ./mingw-test.sh [test-name]
set -e
trap 'rm src/sysinc_paths.h src/gcc_predefined.h' EXIT
WINE_DISABLE_RANDR=1
export WINE_DISABLE_RANDR

if [ -e /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll ] && [ ! -e libwinpthread-1.dll ]; then
    cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll .
fi

if [ -n "${1:-}" ]; then
    make -s CC=x86_64-w64-mingw32-gcc
    ./run_tests.exe ./rcc.exe --no-color "$1"
else
    make leanclean
    make -s CC=x86_64-w64-mingw32-gcc
    echo "==> Running full test suite via mingw-cross.sh..."
    echo ""
    ./run_tests.exe ./rcc.exe --all --no-color
    ./run-c-testsuite.sh ../rcc.exe
fi
