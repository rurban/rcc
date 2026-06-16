#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# RCC vs TCC vs all compilers benchmark (Unix version of run_bench.ps1)
# Usage: ./bench/run_bench.sh [rcc-binary]

set -e

cd "$(dirname "$0")/.." || exit 1
SRC="bench/bench.c"
RCC="${1:-./rcc}"
TCC="${TCC:-tcc}"
GCC="${GCC:-gcc}"
CLANG="$(which clang 2>/dev/null || true)"
KEFIR="$(which kefir 2>/dev/null || true)"
if [ -z "$KEFIR" ] && [ -e "/opt/kefir/bin/kefir" ]; then
   KEFIR="/opt/kefir/bin/kefir"
fi
SLIMCC="$(which slimcc 2>/dev/null || true)"
if [ -z "$SLIMCC" ] && [ -e "bench/../../slimcc/slimcc" ]; then
   SLIMCC="../slimcc/slimcc"
fi
# https://github.com/anthropics/claudes-c-compiler/
CCC="$(which ccc 2>/dev/null || true)"
if [ -z "$CCC" ] && [ -e "../claudes-c-compiler/target/release/ccc" ]; then
   CCC="../claudes-c-compiler/target/release/ccc"
fi

LARGE_SRC="bench/sqlite3.c"
LARGE_SRC_URL="https://sqlite.org/2026/sqlite-amalgamation-3530200.zip"

# download_sqlite: fetch sqlite3.c amalgamation if missing (cached)
download_sqlite() {
	if [ -f "$LARGE_SRC" ]; then
		return 0
	fi
	printf "\nDownloading sqlite amalgamation...\n"
	if command -v curl >/dev/null 2>&1; then
		curl -sSL "$LARGE_SRC_URL" -o /tmp/sqlite-amalg.zip
	elif command -v wget >/dev/null 2>&1; then
		wget -q "$LARGE_SRC_URL" -O /tmp/sqlite-amalg.zip
	else
		printf "  SKIP: no curl or wget\n"
		return 1
	fi
	unzip -o -j /tmp/sqlite-amalg.zip "sqlite-amalgamation-*/sqlite3.c" "sqlite-amalgamation-*/sqlite3.h" -d bench/
	rm -f /tmp/sqlite-amalg.zip
	printf "  Downloaded sqlite3.c (%s lines)\n" "$(wc -l < "$LARGE_SRC")"
}

# Check rcc and tcc exist
if [ ! -x "$RCC" ]; then
    echo "ERROR: rcc not found at '$RCC'. Build it first." >&2
    exit 1
fi
if ! command -v "$TCC" >/dev/null 2>&1; then
    echo "WARNING: tcc not found — skipping TCC benchmark" >&2
    TCC=""
fi

RCC_EXE="bench/bench_rcc"
RCC_O1_EXE="bench/bench_rcc_o1"
TCC_EXE="bench/bench_tcc"
GCC_EXE="bench/bench_gcc"
GCC_O2_EXE="bench/bench_gcc_o2"
CLANG_EXE="bench/bench_clang"
CLANG_O2_EXE="bench/bench_clang_o2"
KEFIR_EXE="bench/bench_kefir"
KEFIR_O1_EXE="bench/bench_kefir_o1"
SLIMCC_EXE="bench/bench_slimcc"
CCC_EXE="bench/bench_ccc"

RUNS=3
if [ "$(uname -s)" = "Darwin" ]; then
	REPORT="bench/bench_report_darwin.md"
else
	REPORT="bench/bench_report.md"
fi

cleanup() {
	rm -f "$RCC_EXE" "$RCC_O1_EXE" "$TCC_EXE" "$GCC_EXE" "$GCC_O2_EXE" "$CLANG_EXE" "$CLANG_O2_EXE"
	rm -f "$KEFIR_EXE" "$SLIMCC_EXE" "$CCC_EXE"
}
trap cleanup EXIT

# time_ms: prints elapsed ms for a command
# Usage: elapsed=$(time_ms cmd args...)
time_ms() {
	# Prefer GNU date (gdate from coreutils on macOS/BSD); otherwise fall back
	# to the system date.  BSD date does not support %N, so verify the output
	# actually contains nanoseconds before using it.
	if command -v gdate >/dev/null 2>&1; then
		_date="gdate"
	elif [ -x /usr/local/bin/gdate ]; then
		_date="/usr/local/bin/gdate"
	elif [ -x /opt/homebrew/bin/gdate ]; then
		_date="/opt/homebrew/bin/gdate"
	else
		_date="date"
	fi
	if _ns=$($_date +%s%N 2>/dev/null) && [ -n "$_ns" ] && [ "$_ns" != "$($_date +%s 2>/dev/null)N" ]; then
		_start=$($_date +%s%N)
		_rc=0
		"$@" >/dev/null || _rc=$?
		_end=$($_date +%s%N)
		echo $(((_end - _start) / 1000000))
		return $_rc
	else
		_start=$($_date +%s)
		_rc=0
		"$@" >/dev/null || _rc=$?
		_end=$($_date +%s)
		echo $(((_end - _start) * 1000))
		return $_rc
	fi
}

# run_bench LABEL COMPILER ARGS EXE
run_bench() {
	_label="$1"
	_compiler="$2"
	_args="$3"
	_exe="$4"
	printf "\n--- %s ---\n" "$_label"
        list_c="$list_c|$_label"

	# Compile
        # shellcheck disable=SC2086
	_compile_ms=$(time_ms $_compiler $_args 2>/dev/null) || true
	if [ ! -x "$_exe" ]; then
		printf "  COMPILE FAILED\n"
		return 1
	fi
	printf "  Compile : %6s ms\n" "$_compile_ms"

	# Execute best of N
	_best=1000
	_output=""
	_i=0
	while [ $_i -lt $RUNS ]; do
		_output="$("$_exe")"
		_exec_ms=$(time_ms "$_exe")
		if [ "$_exec_ms" -lt "$_best" ]; then
		    _best=$_exec_ms
		fi
		_i=$((_i + 1))
	done
        # shellcheck disable=SC2086
        if [ $_best = 1000 ]; then
            _best=$_exec_ms
        fi
	printf "  Execute : %6s ms  (best of %d)\n" "$_best" "$RUNS"
	printf "  Total   : %6s ms\n" $((_compile_ms + _best))

	# Store for scoreboard — replace -/space with _ for safe variable names
	_vname="$(echo "${_label%%(*}" | tr ' -' '__')"
	eval "${_vname}_COMPILE=$_compile_ms"
	eval "${_vname}_EXEC=$_best"
	eval "${_vname}_TOTAL=$((_compile_ms + _best))"
	eval "${_vname}_OUTPUT='$_output'"
	rm -f "$_exe"
	return 0
}

list_c=""

echo ""
echo "============================================"
echo "  RCC substep timing  (-time)"
echo "============================================"
echo ""
printf "\n--- RCC ---\n"
rcc_time=$("$RCC" -time "$SRC" -o "$RCC_EXE" 2>&1 >/dev/null) || true
printf '%s\n' "$rcc_time" | column -t
rm -f "$RCC_EXE"
printf "\n--- RCC -O1 ---\n"
rcc_o1_time=$("$RCC" -time -O1 "$SRC" -o "$RCC_O1_EXE" 2>&1 >/dev/null) || true
printf '%s\n' "$rcc_o1_time" | column -t
rm -f "$RCC_O1_EXE"

# ---- Large file substep timing ----
if download_sqlite; then
    echo ""
    echo "============================================"
    echo "  RCC substep timing  --  sqlite3.c"
    echo "============================================"
    echo ""
    printf "\n--- RCC ---\n"
    rcc_large_time=$("$RCC" -time -c "$LARGE_SRC" -o /dev/null 2>&1 >/dev/null) || true
    printf '%s\n' "$rcc_large_time" | column -t
    printf "\n--- RCC -O1 ---\n"
    rcc_large_o1_time=$("$RCC" -time -O1 -c "$LARGE_SRC" -o /dev/null 2>&1 >/dev/null) || true
    printf '%s\n' "$rcc_large_o1_time" | column -t
fi

echo ""
echo "======================================"
echo "  RCC vs TCC vs others  --  Small file"
echo "======================================"

run_bench "RCC" "$RCC" "$SRC -o $RCC_EXE" "$RCC_EXE"
run_bench "RCC -O1" "$RCC" "-O1 $SRC -o $RCC_O1_EXE" "$RCC_O1_EXE"
if [ -n "$TCC" ]; then
    run_bench "TCC" "$TCC" "$SRC -o $TCC_EXE" "$TCC_EXE" || true
fi
if [ -n "$SLIMCC" ]; then
   run_bench "SLIMCC" "$SLIMCC" "$SRC -o $SLIMCC_EXE" "$SLIMCC_EXE" || true
fi
if [ -n "$KEFIR" ]; then
   run_bench "KEFIR" "$KEFIR" "$SRC -o $KEFIR_EXE" "$KEFIR_EXE" || true
   run_bench "KEFIR -O1" "$KEFIR" "-O1 $SRC -o $KEFIR_O1_EXE" "$KEFIR_O1_EXE" || true
fi
if [ -n "$CCC" ]; then
   run_bench "CCC" "$CCC" "$SRC -o $CCC_EXE" "$CCC_EXE" || true
fi
run_bench "GCC -O0" "$GCC" "-O0 $SRC -o $GCC_EXE -lm" "$GCC_EXE" || true
run_bench "GCC -O2" "$GCC" "-O2 $SRC -o $GCC_O2_EXE -lm" "$GCC_O2_EXE" || true
if [ -n "$CLANG" ]; then
   run_bench "Clang -O0" "$CLANG" "-O0 $SRC -o $CLANG_EXE -lm" "$CLANG_EXE" || true
   run_bench "Clang -O2" "$CLANG" "-O2 $SRC -o $CLANG_O2_EXE -lm" "$CLANG_O2_EXE" || true
fi

echo ""
echo "============================================="
echo "               SCOREBOARD"
echo "============================================="
printf "%-30s %10s %10s %10s\n" "Compiler " "Compile" "Execute" "Total"
printf "%-30s %10s %10s %10s\n" "---------" "-------" "-------" "-----"
oldifs="$IFS"
IFS='|'
for _c in $list_c; do
	[ -z "$_c" ] && continue
	_vname="$(echo "$_c" | tr ' -' '__')"
	eval "_cm=\${${_vname}_COMPILE:-}"
	eval "_em=\${${_vname}_EXEC:-}"
	eval "_tm=\${${_vname}_TOTAL:-}"
	[ -z "$_cm" ] && continue
        # shellcheck disable=SC2154
	printf "%-30s %8s ms %8s ms %8s ms\n" "$_c" "$_cm" "$_em" "$_tm"
done
IFS="$oldifs"

# ---- Large file compile-only benchmark ----
if [ -f "$LARGE_SRC" ]; then
    echo ""
    echo "============================================="
    echo "     LARGE FILE COMPILE-ONLY  (sqlite3.c)"
    echo "============================================="
    printf "%-30s %10s\n" "Compiler" "Compile (ms)"
    printf "%-30s %10s\n" "--------" "-----------"

    large_results=""
    nl='
'
    _compile_large() {
	# shellcheck disable=SC2086
	_label="$1"
	shift
	printf "%-30s " "$_label"
	_rc=0
	_cm=$(time_ms "$@" 2>/dev/null) || _rc=$?
	if [ "$_rc" -ne 0 ]; then
	    printf "    FAIL\n"
	else
	    printf "%8s ms\n" "${_cm:-FAILED}"
	    if [ -n "$_cm" ]; then
		large_results="$large_results$(printf '| %-9s | %12s |' "$_label" "${_cm} ms")$nl"
	    fi
	fi
    }

    _compile_large "RCC" "$RCC" -c "$LARGE_SRC" -o /dev/null
    _compile_large "RCC -O1" "$RCC" -O1 -c "$LARGE_SRC" -o /dev/null
    if [ -n "$TCC" ]; then
	# TCC defines __GNUC__ but doesn't support __uint128_t casts on ARM64
	_compile_large "TCC" "$TCC" -DSQLITE_DISABLE_INTRINSIC -c "$LARGE_SRC" -o /dev/null
    fi
    if [ -n "$SLIMCC" ]; then
	# SLIMCC lacks __atomic_store_n; expected to FAIL
	_compile_large "SLIMCC" "$SLIMCC" -DSQLITE_THREADSAFE=0 -D"__atomic_store_n(x,y,z)" -D"__atomic_load_n(x,z)" -c "$LARGE_SRC" -o /dev/null
    fi
    if [ -n "$KEFIR" ]; then
	_compile_large "KEFIR" "$KEFIR" -c "$LARGE_SRC" -o /dev/null
	_compile_large "KEFIR" "$KEFIR" -O1 -c "$LARGE_SRC" -o /dev/null
    fi
    if [ -n "$CCC" ]; then
	_compile_large "CCC" "$CCC" -c "$LARGE_SRC" -o /dev/null
    fi
    _compile_large "GCC -O0" "$GCC" -O0 -c "$LARGE_SRC" -o /dev/null
    _compile_large "GCC -O2" "$GCC" -O2 -c "$LARGE_SRC" -o /dev/null
    if [ -n "$CLANG" ]; then
	_compile_large "Clang -O0" "$CLANG" -O0 -c "$LARGE_SRC" -o /dev/null
	_compile_large "Clang -O2" "$CLANG" -O2 -c "$LARGE_SRC" -o /dev/null
    fi
fi # LARGE_SRC

# Write markdown report
{
	if [ "$(uname -s)" = "Darwin" ]; then
		printf "# Darwin RCC Benchmark Results\n\n"
	else
		printf "# Linux RCC Benchmark Results\n\n"
	fi
	printf "_Generated: %s_\n\n" "$(date '+%B %Y')"
	printf "| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |\n"
	printf "| :-------- | -----------: | -----------: | ---------: |\n"
IFS='|'
for _c in $list_c; do
	[ -z "$_c" ] && continue
	_vname="$(echo "$_c" | tr ' -' '__')"
	eval "_cm=\${${_vname}_COMPILE:-}"
	eval "_em=\${${_vname}_EXEC:-}"
	eval "_tm=\${${_vname}_TOTAL:-}"
	[ -z "$_cm" ] && continue
	printf "| %-9s | %12s | %12s | %10s |\n" "$_c" "$_cm" "$_em" "$_tm"
done
IFS="$oldifs"
printf "\n## RCC Substep Timing\n\n"
printf '```\n'
printf "RCC:\n"
printf '%s\n' "$rcc_time" | sed 's|[^ ]*/||g'
printf "\nRCC -O1:\n"
printf '%s\n' "$rcc_o1_time" | sed 's|[^ ]*/||g'
printf '```\n'
if [ -n "${rcc_large_time:-}" ]; then
	printf "\n## RCC Substep Timing -- sqlite3.c\n\n"
	printf '```\n'
	printf "RCC:\n"
	printf '%s\n' "$rcc_large_time" | sed 's|[^ ]*/||g'
	printf "\nRCC -O1:\n"
	printf '%s\n' "$rcc_large_o1_time" | sed 's|[^ ]*/||g'
	printf '```\n'
fi
if [ -n "${large_results:-}" ]; then
	printf "\n## Large File Compile-Only (sqlite3.c)\n\n"
	printf "| Compiler  | Compile (ms) |\n"
	printf "| :-------- | -----------: |\n"
	printf '%s' "$large_results"
fi
} > "$REPORT"
printf "Report: %s\n" "$REPORT"

echo ""
echo "ALL DONE"
