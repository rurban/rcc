#!/usr/bin/env bash
# Orchestrate a batch of test/linux_thirdparty.bash package tests with CC=rcc.
# Usage: ./run_batch.sh [test_foo ...]   (default: all targets from targets.txt)
# Env: TIMEOUT per-test seconds (default 420), JOBS parallelism (default 4)
set -eu
ROOT="$(realpath "$(dirname "$0")/../..")"
export CC="$ROOT/rcc"
BASE="$ROOT/test/third_party"
TIMEOUT="${TIMEOUT:-420}"
JOBS="${JOBS:-4}"
mkdir -p "$BASE/logs"

if [ $# -gt 0 ]; then
  targets=("$@")
else
  mapfile -t targets < "$BASE/targets.txt"
fi

: > "$BASE/results.txt"

run_one() {
  local t="$1"
  local d="$BASE/$t"
  rm -rf "$d"; mkdir -p "$d"
  local start=$SECONDS
  ( cd "$d" && timeout "$TIMEOUT" bash "$ROOT/test/linux_thirdparty.bash" "$t" ) \
      > "$BASE/logs/$t.log" 2>&1
  local rc=$?
  local dur=$((SECONDS - start))
  printf '%s\t%s\t%ss\n' "$rc" "$t" "$dur" >> "$BASE/results.txt"
  if [ "$rc" -eq 0 ]; then
    rm -rf "$d"
  fi
}
export -f run_one
export ROOT BASE CC TIMEOUT

printf '%s\n' "${targets[@]}" | xargs -P"$JOBS" -I{} bash -c 'run_one "$@"' _ {}

echo "=== RESULTS (rc  test  dur) ==="
sort -k2 "$BASE/results.txt"
echo "=== SUMMARY ==="
awk -F'\t' '{if($1==0)p++;else f++} END{printf "PASS=%d FAIL=%d\n",p+0,f+0}' "$BASE/results.txt"
