#!/bin/sh
# Cross-build Windows rcc and run the tests against it.
set -e
trap 'rm src/sysinc_paths.h src/gcc_predefined.h' EXIT
make leanclean
make -s CC=x86_64-w64-mingw32-gcc
WINE_DISABLE_RANDR=1
export WINE_DISABLE_RANDR
if [ -e /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll ] && [ ! -e libwinpthread-1.dll ]; then
    cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll .
fi
echo "==> Running full test suite via mingw-cross.sh..."
echo ""
./run_tests.exe ./rcc.exe --all --no-color
./run-c-testsuite.sh ../rcc.exe
