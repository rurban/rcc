#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Install everything needed to build and test the mingw cross target:
#   - x86_64-w64-mingw32-gcc (Linux -> Windows cross-compiler), used to
#     build rcc.exe and lib/rcc_mingw.obj, and as the backend for
#     mingw-cross.sh.
#   - wine, used to run rcc.exe / run_tests.exe and the resulting .exe
#     test binaries.
#   - a Windows-native MinGW-w64 GCC 16.1.0 toolchain (niXman
#     mingw-builds-binaries), installed into the wine prefix as
#     C:\mingw64 and added to wine's PATH. This is the backend that
#     rcc.exe itself shells out to (as "gcc") when run standalone under
#     wine, e.g. `wine ./run_tests.exe rcc.exe <test>`.
#
# Usage: ./tools/install-mingw-cross.sh
set -e

MINGW_VER=16.1.0
MINGW_TAG=16.1.0-rt_v14-rev1
MINGW_URL="https://github.com/niXman/mingw-builds-binaries/releases/download/${MINGW_TAG}/x86_64-${MINGW_VER}-release-win32-seh-msvcrt-${MINGW_TAG#*-}.7z"

echo "==> Installing cross-compiler and wine packages..."
if command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y \
        gcc-mingw-w64-x86-64 binutils-mingw-w64-x86-64 \
        wine p7zip-full
elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y \
        mingw64-gcc mingw64-binutils \
        wine p7zip p7zip-plugins
else
    echo "ERROR: neither apt-get nor dnf found; install manually:" >&2
    echo "  - x86_64-w64-mingw32-gcc / binutils" >&2
    echo "  - wine" >&2
    echo "  - p7zip (7z)" >&2
    exit 1
fi

if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    echo "ERROR: x86_64-w64-mingw32-gcc not found after install" >&2
    exit 1
fi
if ! command -v wine >/dev/null 2>&1; then
    echo "ERROR: wine not found after install" >&2
    exit 1
fi
if ! command -v 7z >/dev/null 2>&1; then
    echo "ERROR: 7z not found after install" >&2
    exit 1
fi

WINEPREFIX="${WINEPREFIX:-$HOME/.wine}"

echo ""
echo "==> Initializing wine prefix at $WINEPREFIX (if needed)..."
WINEPREFIX="$WINEPREFIX" WINEDEBUG=fixme-all wineboot -u >/dev/null 2>&1 || true

if [ -x "$WINEPREFIX/drive_c/mingw64/bin/gcc.exe" ]; then
    echo ""
    echo "==> $WINEPREFIX/drive_c/mingw64 already has gcc.exe, skipping download."
else
    echo ""
    echo "==> Downloading MinGW-w64 GCC $MINGW_VER (Windows-native, ~100MB)..."
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT
    curl -L --fail -o "$tmpdir/mingw64.7z" "$MINGW_URL"

    echo "==> Extracting to $WINEPREFIX/drive_c/mingw64..."
    mkdir -p "$WINEPREFIX/drive_c/mingw64"
    7z x -y -o"$tmpdir/extracted" "$tmpdir/mingw64.7z" >/dev/null
    cp -r "$tmpdir/extracted/mingw64/." "$WINEPREFIX/drive_c/mingw64/"
fi

echo ""
printf '==> Adding C:\\\\mingw64\\\\bin to wine'"'"'s PATH...\n'
WINEPREFIX="$WINEPREFIX" WINEDEBUG=fixme-all wine reg add "HKEY_CURRENT_USER\\Environment" \
    /v PATH /t REG_EXPAND_SZ /d "C:\\mingw64\\bin;%PATH%" /f >/dev/null

echo ""
echo "==> Verifying gcc.exe runs under wine..."
WINEPREFIX="$WINEPREFIX" WINEDEBUG=fixme-all wine cmd /c "gcc --version" 2>/dev/null | head -1

echo ""
echo "==> Done. Build and test the mingw cross target with:"
echo ""
echo "  make CC=x86_64-w64-mingw32-gcc rcc.exe run_tests.exe"
echo "  wine ./run_tests.exe rcc.exe"
echo ""
echo "Or use the slower ./mingw-cross.sh as the rcc command for run_tests:"
echo ""
echo "  ./run_tests ./mingw-cross.sh --tcc"
