#!/usr/bin/env bash
# Quick RCC-only benchmark: timings + binary sizes, updates bench_report.md.
# Usage: ./bench/bench_my.sh [rcc-binary]
set -e
export LC_NUMERIC=C
cd "$(dirname "$0")/.." || exit 1
SRC="bench/bench.c"
RCC="${1:-./rcc}"
RUNS=3
REPORT="bench/bench_report.md"

[ -x "$RCC" ] || { echo "rcc not found: $RCC" >&2; exit 1; }

# Pin to one core for stability
PIN=""
if command -v taskset >/dev/null 2>&1; then
    if taskset -c 7 true 2>/dev/null; then
        PIN="taskset -c 7"
    fi
fi

time_ms() {
    local start end
    start=$(date +%s%N)
    "$@" >/dev/null 2>&1
    end=$(date +%s%N)
    echo $(( (end - start) / 1000000 ))
}

bench_one() {
    local label="$1" flags="$2" exe="bench/bench_my"
    local best_c=999999 best_e=999999 c e
    for i in $(seq 1 $RUNS); do
        c=$(time_ms $PIN $RCC $flags $SRC -o $exe)
        e=$(time_ms $PIN $exe)
        [ $c -lt $best_c ] && best_c=$c
        [ $e -lt $best_e ] && best_e=$e
    done
    local size
    size=$(stat -c%s $exe 2>/dev/null || stat -f%z $exe 2>/dev/null)
    rm -f $exe
    echo "$label|$best_c|$best_e|$(($best_c + $best_e))|$size"
}

echo "Running quick RCC bench ($RUNS runs each)..."
r0=$(bench_one "RCC" "")
r1=$(bench_one "RCC -O1" "-O1")
r2=$(bench_one "RCC -O2" "-O2")

echo ""
printf "%-10s %10s %10s %10s %10s\n" "Compiler" "Compile" "Execute" "Total" "Size(B)"
printf "%-10s %10s %10s %10s %10s\n" "---------" "-------" "-------" "-----" "-------"
for r in "$r0" "$r1" "$r2"; do
    IFS='|' read -r name c e t s <<< "$r"
    printf "%-10s %8d ms %8d ms %8d ms %8d\n" "$name" "$c" "$e" "$t" "$s"
done

# Update bench_report.md: replace RCC rows in the small file table
python3 - "$REPORT" "$r0" "$r1" "$r2" << 'PYEOF'
import sys, re

report = sys.argv[1]
r0, r1, r2 = sys.argv[2], sys.argv[3], sys.argv[4]

def parse(s):
    parts = s.split('|')
    return parts[0], int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4])

l0, c0, e0, t0, s0 = parse(r0)
l1, c1, e1, t1, s1 = parse(r1)
l2, c2, e2, t2, s2 = parse(r2)

with open(report, 'r') as f:
    content = f.read()

# Update the three RCC rows in the first table (small file)
lines = content.split('\n')
out = []
in_table = False
for line in lines:
    if line.startswith('| Compiler') or line.startswith('| :---'):
        in_table = True
        out.append(line)
        continue
    if in_table and line.startswith('| RCC'):
        if re.match(r'\| RCC\s*\|', line):
            out.append(f'| RCC       | {c0:>9} | {e0:>9} | {t0:>9} |    -  |')
        elif re.match(r'\| RCC -O1\s*\|', line):
            out.append(f'| RCC -O1   | {c1:>9} | {e1:>9} | {t1:>9} |    -  |')
        elif re.match(r'\| RCC -O2\s*\|', line):
            out.append(f'| RCC -O2   | {c2:>9} | {e2:>9} | {t2:>9} |    -  |')
        else:
            out.append(line)
    elif in_table and not line.startswith('|') and line.strip() == '':
        in_table = False
        out.append(line)
    else:
        out.append(line)

with open(report, 'w') as f:
    f.write('\n'.join(out))

print(f"Updated {report}")
PYEOF

echo "Done."
