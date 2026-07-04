#!/bin/sh
# Cross-build Windows rcc and run the TCC test suite against it.
set -e
trap 'rm -f src/sysinc_paths.h src/gcc_predefined.h' EXIT
make leanclean
make -s CC=x86_64-w64-mingw32-gcc
WINE_DISABLE_RANDR=1
export WINE_DISABLE_RANDR
if [ -e /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll ] && [ ! -e libwinpthread-1.dll ]; then
    cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll .
fi
timeout 3m ./run_tests.exe ./rcc.exe --tcc --no-color
