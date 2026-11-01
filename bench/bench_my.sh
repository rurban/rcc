#!/usr/bin/env bash
# Quick RCC-only benchmark: all suites (small bench.c, Are-We-Fast-Yet,
# sqlite3.c compile-only, -time substeps) for RCC, RCC -O1, RCC -O2.
# Timings are best-of-N; only the RCC rows/blocks of bench_report.md are
# updated, other compilers' rows are left untouched.
# Usage: ./bench/bench_my.sh [rcc-binary]
# Override sample counts with BENCH_RUNS / BENCH_AWFY_RUNS /
# BENCH_COMPILE_RUNS / BENCH_LARGE_RUNS.
set -e
export LC_NUMERIC=C
cd "$(dirname "$0")/.." || exit 1
SRC="bench/bench.c"
RCC="${1:-./rcc}"
REPORT="bench/bench_report.md"

LARGE_SRC="bench/sqlite3.c"
LARGE_SRC_URL="https://sqlite.org/2026/sqlite-amalgamation-3530200.zip"
LARGE_SO="bench/sqlite3.so"
LARGE_CFLAGS="-shared -fPIC $LARGE_SRC -o $LARGE_SO"

AWFY_DIR="bench/awfy"
AWFY_SRCS="$AWFY_DIR/Benchmark.c $AWFY_DIR/Bounce.c $AWFY_DIR/CD.c $AWFY_DIR/DeltaBlue.c $AWFY_DIR/Havlak.c $AWFY_DIR/Json.c $AWFY_DIR/List.c $AWFY_DIR/Mandelbrot.c $AWFY_DIR/NBody.c $AWFY_DIR/Object.c $AWFY_DIR/Permute.c $AWFY_DIR/Queens.c $AWFY_DIR/RedBlackTree.c $AWFY_DIR/Richards.c $AWFY_DIR/Run.c $AWFY_DIR/Sieve.c $AWFY_DIR/Storage.c $AWFY_DIR/Towers.c $AWFY_DIR/main.c $AWFY_DIR/som/Dictionary.c $AWFY_DIR/som/Random.c $AWFY_DIR/som/Set.c $AWFY_DIR/som/Vector.c"

RUNS="${BENCH_RUNS:-3}"
AWFY_RUNS="${BENCH_AWFY_RUNS:-3}"
COMPILE_RUNS="${BENCH_COMPILE_RUNS:-3}"
LARGE_RUNS="${BENCH_LARGE_RUNS:-2}"
for v in RUNS AWFY_RUNS COMPILE_RUNS LARGE_RUNS; do
    case "${!v}" in ''|*[!0-9]*|0) eval "$v=1" ;; esac
done

LABELS=("RCC" "RCC -O1" "RCC -O2" "RCC -O3")
FLAGS=("" "-O1" "-O2" "-O3")
TAGS=("rcc" "o1" "o2" "o3")

[ -x "$RCC" ] || { echo "rcc not found: $RCC" >&2; exit 1; }

# Pin to one core for stability
PIN=()
if command -v taskset >/dev/null 2>&1 && taskset -c 7 true 2>/dev/null; then
    PIN=(taskset -c 7)
fi

OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

time_ms() {
    local start end
    start=$(date +%s%N)
    "$@" >/dev/null 2>&1 || return 1
    end=$(date +%s%N)
    echo $(( (end - start) / 1000000 ))
}

# sample N cmd...: sets BEST/WORST (ms) over N timed runs; fails if a run fails
sample() {
    local n="$1" i t
    shift
    BEST="" WORST=""
    for ((i = 0; i < n; i++)); do
        t=$(time_ms "${PIN[@]}" "$@") || return 1
        if [ -z "$BEST" ] || [ "$t" -lt "$BEST" ]; then BEST=$t; fi
        if [ -z "$WORST" ] || [ "$t" -gt "$WORST" ]; then WORST=$t; fi
    done
}

pct() { # best worst
    if [ "$1" -gt 0 ]; then echo $(( ($2 - $1) * 100 / $1 )); else echo 0; fi
}

maxi() { if [ "$1" -gt "$2" ]; then echo "$1"; else echo "$2"; fi; }

# compile+execute suite: bench_suite NAME LABEL FLAGS EXE RUNS SRCS...
# appends "label|compile|execute|total|spread" to $OUT/NAME
bench_suite() {
    local name="$1" label="$2" flags="$3" exe="$4" runs="$5" srcs="$6"
    local fl=() sl=() cb cw cp eb ew ep output
    read -ra fl <<< "$flags"
    read -ra sl <<< "$srcs"
    rm -f "$exe"
    if ! sample "$COMPILE_RUNS" "$RCC" "${fl[@]}" "${sl[@]}" -o "$exe" -lm || [ ! -x "$exe" ]; then
        echo "  $label: COMPILE FAILED"
        rm -f "$exe"
        return 0
    fi
    cb=$BEST cw=$WORST
    if [ "$name" = awfy ]; then
        output=$("${PIN[@]}" "$exe" 2>&1 || true)
        if printf '%s' "$output" | grep -qi "failed with incorrect result"; then
            echo "  $label: RESULT INCORRECT"
            rm -f "$exe"
            return 0
        fi
    fi
    if ! sample "$runs" "$exe"; then
        echo "  $label: EXECUTE FAILED"
        rm -f "$exe"
        return 0
    fi
    eb=$BEST ew=$WORST
    rm -f "$exe"
    cp=$(pct "$cb" "$cw")
    ep=$(pct "$eb" "$ew")
    printf "  %-8s compile %6d ms  execute %6d ms  total %6d ms  spread %d%%\n" \
        "$label" "$cb" "$eb" $((cb + eb)) "$(maxi "$cp" "$ep")"
    echo "$label|$cb|$eb|$((cb + eb))|$(maxi "$cp" "$ep")" >> "$OUT/$name"
}

download_sqlite() {
    [ -f "$LARGE_SRC" ] && return 0
    echo "Downloading sqlite amalgamation..."
    if command -v curl >/dev/null 2>&1; then
        curl -sSL "$LARGE_SRC_URL" -o /tmp/sqlite-amalg.zip
    elif command -v wget >/dev/null 2>&1; then
        wget -q "$LARGE_SRC_URL" -O /tmp/sqlite-amalg.zip
    else
        echo "  SKIP: no curl or wget"
        return 1
    fi
    unzip -o -j /tmp/sqlite-amalg.zip "sqlite-amalgamation-*/sqlite3.c" "sqlite-amalgamation-*/sqlite3.h" -d bench/ >/dev/null
    rm -f /tmp/sqlite-amalg.zip
}

echo "== Small file (bench.c), $RUNS exec / $COMPILE_RUNS compile runs =="
for i in 0 1 2 3; do
    bench_suite small "${LABELS[$i]}" "${FLAGS[$i]}" "bench/bench_${TAGS[$i]}" "$RUNS" "$SRC"
done

echo "== Are-We-Fast-Yet, $AWFY_RUNS exec / $COMPILE_RUNS compile runs =="
for i in 0 1 2 3; do
    bench_suite awfy "${LABELS[$i]}" "-std=c99 ${FLAGS[$i]}" "bench/awfy_my_${TAGS[$i]}" "$AWFY_RUNS" "$AWFY_SRCS"
done

echo "== Substep timing (-time) =="
for i in 0 1 2 3; do
    exe="bench/bench_${TAGS[$i]}"
    fl=()
    read -ra fl <<< "${FLAGS[$i]}"
    "${PIN[@]}" "$RCC" -time "${fl[@]}" "$SRC" -o "$exe" 2>"$OUT/time_small_${TAGS[$i]}" >/dev/null || true
    rm -f "$exe"
    echo "--- ${LABELS[$i]} ---"
    cat "$OUT/time_small_${TAGS[$i]}"
done

if download_sqlite; then
    echo "== sqlite3.c substeps =="
    for i in 0 1 2 3; do
        fl=()
        read -ra fl <<< "${FLAGS[$i]}"
        # shellcheck disable=SC2086
        "${PIN[@]}" "$RCC" -time "${fl[@]}" $LARGE_CFLAGS 2>"$OUT/time_large_${TAGS[$i]}" >/dev/null || true
        echo "--- ${LABELS[$i]} ---"
        cat "$OUT/time_large_${TAGS[$i]}"
    done

    echo "== sqlite3.c compile-only, $LARGE_RUNS runs =="
    for i in 0 1 2 3; do
        fl=()
        read -ra fl <<< "${FLAGS[$i]}"
        # shellcheck disable=SC2086
        if sample "$LARGE_RUNS" "$RCC" "${fl[@]}" $LARGE_CFLAGS; then
            printf "  %-8s compile %6d ms  spread %d%%\n" "${LABELS[$i]}" "$BEST" "$(pct "$BEST" "$WORST")"
            echo "${LABELS[$i]}|$BEST|$(pct "$BEST" "$WORST")" >> "$OUT/large"
        else
            echo "  ${LABELS[$i]}: FAIL"
        fi
    done
    rm -f "$LARGE_SO"
fi

# Update the RCC rows/blocks in bench_report.md, section by section
python3 - "$REPORT" "$OUT" << 'PYEOF'
import sys, re, os, datetime

report, out = sys.argv[1], sys.argv[2]

def rows(name):
    p = os.path.join(out, name)
    if not os.path.exists(p):
        return {}
    d = {}
    for line in open(p).read().splitlines():
        parts = line.split('|')
        d[parts[0]] = parts[1:]
    return d

def timing(kind):
    res = {}
    for label, tag in (('RCC', 'rcc'), ('RCC -O1', 'o1'), ('RCC -O2', 'o2'), ('RCC -O3', 'o3')):
        p = os.path.join(out, f'time_{kind}_{tag}')
        if os.path.exists(p):
            res[label] = re.sub(r'[^ ]*/', '', open(p).read().rstrip('\n'))
    return res

small, awfy, large = rows('small'), rows('awfy'), rows('large')
tsmall, tlarge = timing('small'), timing('large')

def full_row(label, v):
    c, e, t, s = v
    return f'| {label:<9} | {c:>12} | {e:>12} | {t:>10} | {s:>5}% |'

def large_row(label, v):
    return f'| {label:<9} | {v[0] + " ms":>12} | {v[1] + "%":>6} |'

row_re = re.compile(r'\| (RCC(?: -O[123])?)\s*\|')
lines = open(report).read().split('\n')
res = []
section = ''
block_for = None   # substep block pending replacement
i = 0
while i < len(lines):
    line = lines[i]
    if line.startswith('_Generated:'):
        line = f"_Generated: {datetime.date.today().strftime('%B %Y')}_"
    if line.startswith('## '):
        section = line
        block_for = None
        if line == '## RCC Substep Timing' and tsmall:
            block_for = tsmall
        elif line == '## RCC Substep Timing -- sqlite3.c' and tlarge:
            block_for = tlarge
    elif block_for is not None and line == '```':
        # replace the whole fenced block
        j = i + 1
        while lines[j] != '```':
            j += 1
        body = []
        for label in ('RCC', 'RCC -O1', 'RCC -O2', 'RCC -O3'):
            if label in block_for:
                body.append(f'{label}:\n{block_for[label]}')
        res.append('```')
        res.append('\n\n'.join(body))
        res.append('```')
        i = j + 1
        block_for = None
        continue
    m = row_re.match(line)
    if m:
        label = m.group(1)
        if section.startswith('## Are-We-Fast-Yet'):
            if label in awfy:
                line = full_row(label, awfy[label])
        elif section.startswith('## Large File'):
            if label in large:
                line = large_row(label, large[label])
        elif section == '':
            if label in small:
                line = full_row(label, small[label])
    elif section.startswith('## Large File') and line.startswith('| :--'):
        line = '| :-------- | -----------: | -----: |'
    res.append(line)
    i += 1

with open(report, 'w') as f:
    f.write('\n'.join(res))
print(f"Updated {report}")
PYEOF

# Format report for CI lint (match CI prettier version)
if command -v npx >/dev/null 2>&1; then
    npx prettier@3.6.2 --write "$REPORT" >/dev/null 2>&1 || true
fi

echo "Done."
