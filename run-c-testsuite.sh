#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
cd "$(dirname "$0")" || exit

if [ ! -f c-testsuite/single-exec ]; then
    git submodule update --init --recursive
fi

if [ -f c-testsuite/fred.txt ] && command -v git >/dev/null 2>&1; then
    echo "Clean c-testsuite"
    cd c-testsuite && git clean -dxf . && cd ..
fi

TO_TMP=""
if ! command -v timeout >/dev/null 2>&1; then
    if command -v gtimeout >/dev/null 2>&1; then
        TO_TMP=$(mktemp -d)
        ln -s "$(command -v gtimeout)" "$TO_TMP/timeout"
        export PATH="$TO_TMP:$PATH"
    else
        # no timeout available, emulate with a wrapper
        TO_TMP=$(mktemp -d)
        cat > "$TO_TMP/timeout" << 'WRAPPER'
#!/bin/sh
# Emulate GNU timeout: convert duration suffix to seconds
dur="${1}"; shift
case "$dur" in
    *s) dur="${dur%s}" ;;
    *m) dur=$(( ${dur%m} * 60 )) ;;
    *h) dur=$(( ${dur%h} * 3600 )) ;;
    *d) dur=$(( ${dur%d} * 86400 )) ;;
esac
"$@" &
pid=$!
(sleep "$dur" && kill -TERM "$pid" 2>/dev/null && sleep 1 && kill -KILL "$pid" 2>/dev/null) &
wait "$pid"
WRAPPER
        chmod +x "$TO_TMP/timeout"
        export PATH="$TO_TMP:$PATH"
    fi
fi

cd c-testsuite || exit
# Detect native platform for summary filename
detect_platform() {
    case "$(uname -s)" in
        Linux)  echo "linux" ;;
        Darwin) echo "arm64" ;;
        MINGW*|MSYS*|CYGWIN*) echo "mingw" ;;
        *)      echo "linux" ;;
    esac
}
platform="$(detect_platform)"
if [ "$1" = "../rcc.exe" ]; then
    if [ -f runners/single-exec/win ]; then
        sed -e 's,\.bin,\.exe,' <runners/single-exec/posix >runners/single-exec/win
        chmod +x runners/single-exec/win
    fi
    echo "Start c-testsuite with ../rcc.exe -O1 -lm"
    env CC="../rcc.exe" CFLAGS="-O1 -lm" ./single-exec win | scripts/tapsummary | tee ../c-testsuite.tap.txt
else
    echo "Start c-testsuite with ../rcc -O1 -lm"
    env CC="../rcc" CFLAGS="-O1 -lm" ./single-exec posix | scripts/tapsummary | tee ../c-testsuite.tap.txt
fi

MAX_FAILS=0
fails=$(grep -m1 '^fail ' ../c-testsuite.tap.txt | awk '{print $2}')
if [ -z "$fails" ]; then
    echo "ERROR: could not determine test fail count"
    exit 1
fi
if [ "$fails" -gt "$MAX_FAILS" ]; then
    echo "FAIL: got $fails failures, maximum allowed is $MAX_FAILS"
    exit 1
fi
echo "OK: $fails failures, within limit of $MAX_FAILS"

# Write machine-readable summary for unified report (native platform only)
{
    printf 'SUITE=c-testsuite\n'
    awk '{if($1=="pass") printf "PASS=%d\n",$2; if($1=="fail") printf "FAIL=%d\n",$2; if($1=="skip") printf "SKIP=%d\n",$2; if($1=="total") printf "TOTAL=%d\n",$2}' ../c-testsuite.tap.txt
} > "../test-ctest-${platform}.summary"

if [ -n "$TO_TMP" ]; then
    rm -rf "$TO_TMP"
fi
