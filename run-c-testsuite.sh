#!/bin/sh
cd "$(dirname "$0")" || exit
if [ ! -f c-testsuite/single-exec ]; then
    git submodule update --init --recursive
fi

TO_TMP=""
if ! command -v timeout >/dev/null 2>&1; then
    if command -v gtimeout >/dev/null 2>&1; then
        TO_TMP=$(mktemp -d)
        ln -s "$(command -v gtimeout)" "$TO_TMP/timeout"
        export PATH="$TO_TMP:$PATH"
    fi
fi

cd c-testsuite || exit
echo "c-testsuite with ../rcc -O1 -lm"
env CC="../rcc" CFLAGS="-O1 -lm" ./single-exec posix | scripts/tapsummary | tee ../c-testsuite.tap.txt

if [ -n "$TO_TMP" ]; then
    rm -rf "$TO_TMP"
fi
