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

# A crashing test must not open winedbg / a crash dialog: neutralize the prefix's AeDebug
# handler (its default is `winedbg --auto`, which is not stopped by the overrides above).
wine reg add "HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug" /v Debugger /t REG_SZ /d false /f >/dev/null 2>&1 || true
wine reg add "HKCU\\Software\\Wine\\WineDbg" /v ShowCrashDialog /t REG_DWORD /d 0 /f >/dev/null 2>&1 || true

if [ -e /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll ] && [ ! -e libwinpthread-1.dll ]; then
    cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll .
fi

make leanclean
make CC=x86_64-w64-mingw32-gcc
if [ -n "${1:-}" ]; then
    ./run_tests.exe ./rcc.exe "$@"
else
    echo "==> Running full test suite in parallel in_proc via run_tests.exe..."
    echo ""
    ./run_tests.exe ./rcc.exe --all --parallel
fi
