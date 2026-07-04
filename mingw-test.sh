#!/bin/sh
# Cross-build Windows rcc and run the tests against it.
# Pre-requisites: tools/install-mingw-cross.sh
# Usage: ./mingw-test.sh [test-name]
set -e
trap 'rm src/sysinc_paths.h src/gcc_predefined.h' EXIT
WINE_DISABLE_RANDR=1
WINEDEBUG=fixme-all
WINEDLLOVERRIDES="winedbg=d;dbghelp=d;mscoree=d;mshtml=d"
WINENOPOPUPS=1
WINE_DISABLE_CRASH_DIALOG=1
export WINEDEBUG WINEDLLOVERRIDES WINENOPOPUPS WINE_DISABLE_RANDR WINE_DISABLE_CRASH_DIALOG

if [ -e /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll ] && [ ! -e libwinpthread-1.dll ]; then
    cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll .
fi

if [ -n "${1:-}" ]; then
    make -s CC=x86_64-w64-mingw32-gcc
    ./run_tests.exe ./rcc.exe "$@"
else
    make leanclean
    make -s CC=x86_64-w64-mingw32-gcc
    echo "==> Running full test suite sequentially in_proc via run_tests.exe..."
    echo ""
    ./run_tests.exe ./rcc.exe --all
fi
