#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
# RCC vs TCC vs all compilers benchmark (Unix version of run_bench.ps1)
# Usage: ./bench/run_bench.sh [rcc-binary]
#
# Benchmarking method: every timed compile/execute is sampled COMPILE_RUNS
# / RUNS times and reported as best-of-N with the worst-to-best spread (a
# single sample is dominated by cold page-cache and scheduler noise, not
# the compiler's actual cost -- same rationale as the rdtsc/cntvct phase
# timer in src/main.c's -time). Every benched process is pinned to one
# fixed CPU core (taskset) so cross-core migration doesn't add to that
# noise, and where available a fork-free bash EPOCHREALTIME read replaces
# the two `date` subprocess spawns previously sitting inside the bracket.
# Override sample counts with BENCH_RUNS / BENCH_COMPILE_RUNS /
# BENCH_LARGE_RUNS.

set -e
# EPOCHREALTIME (used by time_ms below) is formatted per LC_NUMERIC --
# under e.g. de_DE it prints a comma decimal separator, which breaks the
# `.`-based split and aborts the arithmetic. Force the POSIX radix point.
export LC_NUMERIC=C

cd "$(dirname "$0")/.." || exit 1
SRC="bench/bench.c"
RCC="${1:-./rcc}"
TCC="${TCC:-tcc}"
GCC="${GCC:-gcc}"
CLANG="$(which clang 2>/dev/null || true)"
SLIMCC="$(which slimcc 2>/dev/null || true)"
if [ -z "$SLIMCC" ] && [ -e "bench/../../slimcc/slimcc" ]; then
   SLIMCC="../slimcc/slimcc"
fi
XCC="$(which xcc 2>/dev/null || true)"
if [ -z "$XCC" ] && [ -e "bench/../../xcc/xcc" ]; then
   XCC="../xcc/xcc"
fi
KEFIR="$(which kefir 2>/dev/null || true)"
if [ -z "$KEFIR" ] && [ -e "/opt/kefir/bin/kefir" ]; then
   KEFIR="/opt/kefir/bin/kefir"
fi
# https://github.com/anthropics/claudes-c-compiler/
CCC="$(which ccc 2>/dev/null || true)"
if [ -z "$CCC" ] && [ -e "../claudes-c-compiler/target/release/ccc" ]; then
   CCC="../claudes-c-compiler/target/release/ccc"
fi
# blitzy-c-compiler: https://github.com/blitzy-public-samples/blitzy-c-compiler
# INSTALL:
#  gh repo clone https://github.com/rurban/blitzy-c-compiler/
#  cd blitzy-c-compiler; cargo build --release
# Deliberately NOT resolved via PATH: bcc locates its builtin headers
# (stdarg.h, stddef.h, ...) relative to its own binary path
# (exe_dir/../../include), so a PATH/cargo-install copy with no include/
# next to it fails to preprocess anything. Only the sibling-repo build
# below has that include/ dir alongside it.
if [ -z "$BCC" ] && [ -e "../blitzy-c-compiler/target/release/bcc" ]; then
   BCC="../blitzy-c-compiler/target/release/bcc"
fi
CPROC="$(which cproc 2>/dev/null || true)"
if [ -z "$CPROC" ] && [ -x ../cproc/cproc ]; then
	CPROC="../cproc/cproc"
fi
SCC="$(which scc 2>/dev/null || true)"
if [ -z "$SCC" ] && [ -x ../scc/bin/scc ]; then
	SCC="../scc/bin/scc"
fi
LACC="$(which lacc 2>/dev/null || true)"
if [ -z "$LACC" ] && [ -x ../lacc/bin/lacc ]; then
	LACC="../lacc/bin/lacc"
fi
ANTCC="$(which antcc 2>/dev/null || true)"
if [ -z "$ANTCC" ] && [ -x ../antcc/antcc ]; then
	ANTCC="../antcc/antcc"
fi
CHIBICC="$(which chibicc 2>/dev/null || true)"
if [ -z "$CHIBICC" ] && [ -x ../chibicc/chibicc ]; then
	CHIBICC="../chibicc/chibicc"
fi
CAKE_CC="$(which cake-cc.sh 2>/dev/null || true)"
if [ -z "$CAKE_CC" ] && [ -x ./cake-cc.sh ]; then
	CAKE_CC="./cake-cc.sh"
fi

LARGE_SRC="bench/sqlite3.c"
LARGE_SRC_URL="https://sqlite.org/2026/sqlite-amalgamation-3530200.zip"
LARGE_SO="bench/sqlite3.so"
LARGE_CFLAGS="-shared -fPIC $LARGE_SRC -o $LARGE_SO"

AWFY_DIR="bench/awfy"
AWFY_SRCS="$AWFY_DIR/Benchmark.c $AWFY_DIR/Bounce.c $AWFY_DIR/CD.c $AWFY_DIR/DeltaBlue.c $AWFY_DIR/Havlak.c $AWFY_DIR/Json.c $AWFY_DIR/List.c $AWFY_DIR/Mandelbrot.c $AWFY_DIR/NBody.c $AWFY_DIR/Object.c $AWFY_DIR/Permute.c $AWFY_DIR/Queens.c $AWFY_DIR/RedBlackTree.c $AWFY_DIR/Richards.c $AWFY_DIR/Run.c $AWFY_DIR/Sieve.c $AWFY_DIR/Storage.c $AWFY_DIR/Towers.c $AWFY_DIR/main.c $AWFY_DIR/som/Dictionary.c $AWFY_DIR/som/Random.c $AWFY_DIR/som/Set.c $AWFY_DIR/som/Vector.c"

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
RCC_O1_EXE="bench/bench_o1"
RCC_O2_EXE="bench/bench_o2"
TCC_EXE="bench/bench_tcc"
SLIMCC_EXE="bench/bench_slimcc"
XCC_EXE="bench/bench_xcc"
KEFIR_EXE="bench/bench_kefir"
KEFIR_O1_EXE="bench/bench_kefir_o1"
CPROC_EXE="bench/bench_cproc"
SCC_EXE="bench/bench_scc"
LACC_EXE="bench/bench_lacc"
ANTCC_EXE="bench/bench_antcc"
CAKE_EXE="bench/bench_cake"
CCC_EXE="bench/bench_ccc"
BCC_EXE="bench/bench_bcc"
GCC_EXE="bench/bench_gcc"
GCC_O2_EXE="bench/bench_gcc_o2"
CLANG_EXE="bench/bench_clang"
CLANG_O2_EXE="bench/bench_clang_o2"

# Sample counts: execute is cheap to repeat for the small bench.c file
# (ms-scale) so it gets more samples; the AWFY suite's execute is
# seconds-scale per run, so it keeps the original (smaller) count to
# bound total wall-clock cost. Compile and the whole-sqlite3.c compile
# are progressively more expensive to repeat, so they get progressively
# fewer samples.
RUNS="${BENCH_RUNS:-3}"
AWFY_RUNS="${BENCH_AWFY_RUNS:-3}"
COMPILE_RUNS="${BENCH_COMPILE_RUNS:-3}"
LARGE_RUNS="${BENCH_LARGE_RUNS:-2}"
case "$RUNS" in ''|*[!0-9]*) RUNS=5 ;; esac
case "$AWFY_RUNS" in ''|*[!0-9]*) AWFY_RUNS=3 ;; esac
case "$COMPILE_RUNS" in ''|*[!0-9]*) COMPILE_RUNS=3 ;; esac
case "$LARGE_RUNS" in ''|*[!0-9]*) LARGE_RUNS=2 ;; esac
[ "$RUNS" -lt 1 ] && RUNS=1
[ "$AWFY_RUNS" -lt 1 ] && AWFY_RUNS=1
[ "$COMPILE_RUNS" -lt 1 ] && COMPILE_RUNS=1
[ "$LARGE_RUNS" -lt 1 ] && LARGE_RUNS=1
if [ "$(uname -s)" = "Darwin" ]; then
	REPORT="bench/bench_report_darwin.md"
else
	REPORT="bench/bench_report.md"
fi

# Pin every benched process to one fixed CPU core: migration between
# cores (differing turbo/cache state, other cores' load) is a major
# wall-clock jitter source on a shared/loaded box, and is applied
# uniformly to every compiler under test so it stays fair. Self-test
# before adopting -- sandboxes/containers can reject sched_setaffinity.
PIN=""
if command -v taskset >/dev/null 2>&1; then
	_ncpu="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
	_pin_core=$((_ncpu - 1))
	[ "$_pin_core" -lt 0 ] && _pin_core=0
	if taskset -c "$_pin_core" true >/dev/null 2>&1; then
		PIN="taskset -c $_pin_core"
		printf "Pinned benchmark processes to CPU core %s\n" "$_pin_core"
	fi
fi

# The gortex code-index daemon reindexes on every source edit and can sit at
# ~35% CPU, which both slows the benchmark and wrecks its repeatability
# (observed wall-clock spread: 47% with it running vs 6% with it paused).
# SIGSTOP/SIGCONT rather than stop/start: pausing keeps the warm index, so a
# bench run does not cost a full re-warm afterwards.
GORTEX_PIDS=""

pause_gortex() {
	command -v pgrep >/dev/null 2>&1 || return 0
	GORTEX_PIDS="$(pgrep -x gortex 2>/dev/null || true)"
	[ -n "$GORTEX_PIDS" ] || return 0
	# shellcheck disable=SC2086
	kill -STOP $GORTEX_PIDS 2>/dev/null || true
	printf "Paused gortex daemon while benchmarking (pid: %s)\n" "$(echo "$GORTEX_PIDS" | tr '\n' ' ')"
}

resume_gortex() {
	[ -n "$GORTEX_PIDS" ] || return 0
	# shellcheck disable=SC2086
	kill -CONT $GORTEX_PIDS 2>/dev/null || true
	GORTEX_PIDS=""
}

cleanup() {
	rm -f "$RCC_EXE" "$RCC_O1_EXE" "$RCC_O2_EXE" "$TCC_EXE" "$GCC_EXE" "$GCC_O2_EXE" "$CLANG_EXE" "$CLANG_O2_EXE" "$CPROC_EXE" "$SCC_EXE" "$LACC_EXE" "$ANTCC_EXE" "$CAKE_EXE"

	rm -f "$KEFIR_EXE" "$SLIMCC_EXE" "$XCC_EXE" "$CCC_EXE" "$BCC_EXE" "$LARGE_SO"
	# Must run on every exit path: a paused daemon left behind is worse than a
	# noisy benchmark.
	resume_gortex
}
trap cleanup EXIT
# EXIT alone does not fire on a signal in all shells; resume explicitly, then
# re-raise so the caller still sees a signal death.
trap 'cleanup; trap - INT; kill -INT $$' INT
trap 'cleanup; trap - TERM; kill -TERM $$' TERM
trap 'cleanup; trap - HUP; kill -HUP $$' HUP

pause_gortex

# time_ms: prints elapsed ms for a command
# Usage: elapsed=$(time_ms cmd args...)
time_ms() {
	# Bash 5+: EPOCHREALTIME is a shell variable, no subprocess -- avoids
	# the two `date` forks (and their own scheduling jitter) that would
	# otherwise sit inside the measurement bracket. Falls back to `date`
	# under bash <5, or when invoked via a plain POSIX sh.
	if [ -n "${BASH_VERSINFO:-}" ] && [ "${BASH_VERSINFO[0]}" -ge 5 ]; then
		_s="$EPOCHREALTIME"
		_rc=0
		"$@" >/dev/null || _rc=$?
		_e="$EPOCHREALTIME"
		_s_us=$(( ${_s%.*} * 1000000 + 10#${_s#*.} ))
		_e_us=$(( ${_e%.*} * 1000000 + 10#${_e#*.} ))
		echo $(((_e_us - _s_us) / 1000))
		return $_rc
	fi
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

	# Compile: best-of-N, like execute below. A single compile sample is
	# dominated by cold page-cache/first-touch faults, not the compiler's
	# actual cost. Keep the exe from the LAST iteration for execute.
	_cbest=""
	_cworst=""
	_i=0
	while [ "$_i" -lt "$COMPILE_RUNS" ]; do
		_i=$((_i + 1))
		# shellcheck disable=SC2086
		_compile_ms=$(time_ms $PIN $_compiler $_args 2>/dev/null) || true
		if [ ! -x "$_exe" ]; then
			printf "  COMPILE FAILED\n"
			return 1
		fi
		if [ -z "$_cbest" ] || [ "$_compile_ms" -lt "$_cbest" ]; then _cbest=$_compile_ms; fi
		if [ -z "$_cworst" ] || [ "$_compile_ms" -gt "$_cworst" ]; then _cworst=$_compile_ms; fi
		[ "$_i" -lt "$COMPILE_RUNS" ] && rm -f "$_exe"
	done
	_compile_ms=$_cbest
	_cpct=0
	[ "$_cbest" -gt 0 ] && _cpct=$(( (_cworst - _cbest) * 100 / _cbest ))
	printf "  Compile : %6s ms  (best of %d, spread %d%%)\n" "$_compile_ms" "$COMPILE_RUNS" "$_cpct"
	[ "$_cpct" -ge 20 ] && printf "  WARNING : compile timing unstable (worst %s ms)\n" "$_cworst"

	# Execute best of N
	_best=""
	_worst=""
	_output=""
	_i=0
	while [ "$_i" -lt "$RUNS" ]; do
                # shellcheck disable=SC2086
		_output="$($PIN "$_exe")"
                # shellcheck disable=SC2086
		_exec_ms=$(time_ms $PIN "$_exe")
		if [ -z "$_best" ] || [ "$_exec_ms" -lt "$_best" ]; then _best=$_exec_ms; fi
		if [ -z "$_worst" ] || [ "$_exec_ms" -gt "$_worst" ]; then _worst=$_exec_ms; fi
		_i=$((_i + 1))
	done
	_epct=0
	[ "$_best" -gt 0 ] && _epct=$(( (_worst - _best) * 100 / _best ))
	printf "  Execute : %6s ms  (best of %d, spread %d%%)\n" "$_best" "$RUNS" "$_epct"
	[ "$_epct" -ge 20 ] && printf "  WARNING : execute timing unstable (worst %s ms)\n" "$_worst"
	printf "  Total   : %6s ms\n" $((_compile_ms + _best))

	# Store for scoreboard — replace -/space with _ for safe variable names
	_vname="$(echo "${_label%%(*}" | tr ' -' '__')"
	eval "${_vname}_COMPILE=$_compile_ms"
	eval "${_vname}_COMPILE_PCT=$_cpct"
	eval "${_vname}_EXEC=$_best"
	eval "${_vname}_EXEC_PCT=$_epct"
	eval "${_vname}_TOTAL=$((_compile_ms + _best))"
	eval "${_vname}_OUTPUT='$_output'"
	rm -f "$_exe"
	return 0
}

# run_bench_awfy LABEL COMPILER FLAGS EXE
# Like run_bench, but for the multi-file Are-We-Fast-Yet suite
# (bench/awfy): builds all 14 benchmarks + som/ support library into
# one binary, times compile + best-of-N execute of the full suite
# (main.c's runAll()), and additionally verifies none of the 14
# benchmarks reported a self-check failure.
run_bench_awfy() {
	_label="$1"
	_compiler="$2"
	_flags="$3"
	_exe="$4"
	printf "\n--- %s ---\n" "$_label"
	list_awfy="$list_awfy|$_label"

	# Compile: best-of-N, same rationale as run_bench.
	_cbest=""
	_cworst=""
	_i=0
	while [ "$_i" -lt "$COMPILE_RUNS" ]; do
		_i=$((_i + 1))
		# shellcheck disable=SC2086
		_compile_ms=$(time_ms $PIN $_compiler $_flags $AWFY_SRCS -o "$_exe" -lm 2>/dev/null) || true
		if [ ! -x "$_exe" ]; then
			printf "  COMPILE FAILED\n"
			return 1
		fi
		if [ -z "$_cbest" ] || [ "$_compile_ms" -lt "$_cbest" ]; then _cbest=$_compile_ms; fi
		if [ -z "$_cworst" ] || [ "$_compile_ms" -gt "$_cworst" ]; then _cworst=$_compile_ms; fi
		[ "$_i" -lt "$COMPILE_RUNS" ] && rm -f "$_exe"
	done
	_compile_ms=$_cbest
	_cpct=0
	[ "$_cbest" -gt 0 ] && _cpct=$(( (_cworst - _cbest) * 100 / _cbest ))
	printf "  Compile : %6s ms  (best of %d, spread %d%%)\n" "$_compile_ms" "$COMPILE_RUNS" "$_cpct"
	[ "$_cpct" -ge 20 ] && printf "  WARNING : compile timing unstable (worst %s ms)\n" "$_cworst"

	# Execute best of N (AWFY_RUNS: each run is seconds-scale, unlike the
	# small-file suite, so this stays smaller than RUNS to bound cost)
	_best=""
	_worst=""
	_output=""
	_i=0
	while [ "$_i" -lt "$AWFY_RUNS" ]; do
                # shellcheck disable=SC2086
		_output="$($PIN "$_exe" 2>&1)"
                # shellcheck disable=SC2086
		_exec_ms=$(time_ms $PIN "$_exe")
		if [ -z "$_best" ] || [ "$_exec_ms" -lt "$_best" ]; then _best=$_exec_ms; fi
		if [ -z "$_worst" ] || [ "$_exec_ms" -gt "$_worst" ]; then _worst=$_exec_ms; fi
		_i=$((_i + 1))
	done
	if printf '%s' "$_output" | grep -qi "failed with incorrect result"; then
		printf "  RESULT  : INCORRECT\n"
		rm -f "$_exe"
		return 1
	fi
	_epct=0
	[ "$_best" -gt 0 ] && _epct=$(( (_worst - _best) * 100 / _best ))
	printf "  Execute : %6s ms  (best of %d, spread %d%%)\n" "$_best" "$AWFY_RUNS" "$_epct"
	[ "$_epct" -ge 20 ] && printf "  WARNING : execute timing unstable (worst %s ms)\n" "$_worst"
	printf "  Total   : %6s ms\n" $((_compile_ms + _best))

	_vname="AWFY_$(echo "${_label%%(*}" | tr ' -' '__')"
	eval "${_vname}_COMPILE=$_compile_ms"
	eval "${_vname}_COMPILE_PCT=$_cpct"
	eval "${_vname}_EXEC=$_best"
	eval "${_vname}_EXEC_PCT=$_epct"
	eval "${_vname}_TOTAL=$((_compile_ms + _best))"
	rm -f "$_exe"
	return 0
}

list_awfy=""

list_c=""

echo ""
echo "============================================"
echo "  RCC substep timing  (-time)"
echo "============================================"
echo ""
printf "\n--- RCC ---\n"
rcc_time=$($PIN "$RCC" -time "$SRC" -o "$RCC_EXE" 2>&1 >/dev/null) || true
printf '%s\n' "$rcc_time" | column -t
rm -f "$RCC_EXE"
printf "\n--- RCC -O1 ---\n"
rcc_o1_time=$($PIN "$RCC" -time -O1 "$SRC" -o "$RCC_O1_EXE" 2>&1 >/dev/null) || true
printf '%s\n' "$rcc_o1_time" | column -t
rm -f "$RCC_O1_EXE"
printf "\n--- RCC -O2 ---\n"
rcc_o2_time=$($PIN "$RCC" -time -O2 "$SRC" -o "$RCC_O2_EXE" 2>&1 >/dev/null) || true
printf '%s\n' "$rcc_o2_time" | column -t
rm -f "$RCC_O2_EXE"

# ---- Large file substep timing ----
if download_sqlite; then
    echo ""
    echo "============================================"
    echo "  RCC substep timing  --  sqlite3.c"
    echo "============================================"
    echo ""
    printf "\n--- RCC ---\n"
    # shellcheck disable=SC2086
    rcc_large_time=$($PIN "$RCC" -time $LARGE_CFLAGS 2>&1 >/dev/null) || true
    printf '%s\n' "$rcc_large_time" | column -t
    printf "\n--- RCC -O1 ---\n"
    # shellcheck disable=SC2086
    rcc_large_o1_time=$($PIN "$RCC" -time -O1 $LARGE_CFLAGS 2>&1 >/dev/null) || true
    printf '%s\n' "$rcc_large_o1_time" | column -t
    printf "\n--- RCC -O2 ---\n"
    # shellcheck disable=SC2086
    rcc_large_o2_time=$($PIN "$RCC" -time -O2 $LARGE_CFLAGS 2>&1 >/dev/null) || true
    printf '%s\n' "$rcc_large_o2_time" | column -t
fi

echo ""
echo "======================================"
echo "  RCC vs TCC vs others  --  Small file"
echo "======================================"

run_bench "RCC" "$RCC" "$SRC -o $RCC_EXE" "$RCC_EXE"
run_bench "RCC -O1" "$RCC" "-O1 $SRC -o $RCC_O1_EXE" "$RCC_O1_EXE"
run_bench "RCC -O2" "$RCC" "-O2 $SRC -o $RCC_O2_EXE" "$RCC_O2_EXE"
if [ -n "$TCC" ]; then
    run_bench "TCC" "$TCC" "$SRC -o $TCC_EXE" "$TCC_EXE" || true
fi
if [ -n "$SLIMCC" ]; then
   run_bench "SLIMCC" "$SLIMCC" "$SRC -o $SLIMCC_EXE" "$SLIMCC_EXE" || true
fi
if [ -n "$XCC" ]; then
   run_bench "XCC" "$XCC" "$SRC -o $XCC_EXE" "$XCC_EXE" || true
fi
if [ -n "$KEFIR" ]; then
   run_bench "KEFIR" "$KEFIR" "$SRC -o $KEFIR_EXE" "$KEFIR_EXE" || true
   run_bench "KEFIR -O1" "$KEFIR" "-O1 $SRC -o $KEFIR_O1_EXE" "$KEFIR_O1_EXE" || true
fi
if [ -n "$SCC" ]; then
   run_bench "SCC" "$CCC" "$SRC -o $SCC_EXE" "$SCC_EXE" || true
fi
if [ -n "$LACC" ]; then
   run_bench "LACC" "$LACC" "$SRC -o $LACC_EXE" "$LACC_EXE" || true
fi
if [ -n "$CPROC" ]; then
   run_bench "CPROC" "$CPROC" "$SRC -o $CPROC_EXE" "$CPROC_EXE" || true
fi
if [ -n "$ANTCC" ]; then
   run_bench "ANTCC" "$ANTCC" "$SRC -o $ANTCC_EXE" "$ANTCC_EXE" || true
fi
if [ -n "$CAKE_CC" ]; then
   run_bench "CAKE" "$CAKE_CC" "$SRC -o $CAKE_EXE" "$CAKE_EXE" || true
fi
if [ -n "$BCC" ]; then
   run_bench "BCC" "$BCC" "$SRC -o $BCC_EXE" "$BCC_EXE" || true
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
printf "%-30s %10s %10s %10s %8s\n" "Compiler " "Compile" "Execute" "Total" "Spread"
printf "%-30s %10s %10s %10s %8s\n" "---------" "-------" "-------" "-----" "------"
oldifs="$IFS"
IFS='|'
for _c in $list_c; do
	[ -z "$_c" ] && continue
	_vname="$(echo "$_c" | tr ' -' '__')"
	eval "_cm=\${${_vname}_COMPILE:-}"
	eval "_em=\${${_vname}_EXEC:-}"
	eval "_tm=\${${_vname}_TOTAL:-}"
	eval "_cp=\${${_vname}_COMPILE_PCT:-0}"
	eval "_ep=\${${_vname}_EXEC_PCT:-0}"
	[ -z "$_cm" ] && continue
        # shellcheck disable=SC2154
	_sp=$_cp
        # shellcheck disable=SC2154
	[ "$_ep" -gt "$_sp" ] && _sp=$_ep
	_spstr="${_sp}%"
	[ "$_sp" -ge 20 ] && _spstr="${_sp}%!"
        # shellcheck disable=SC2154
	printf "%-30s %8s ms %8s ms %8s ms %8s\n" "$_c" "$_cm" "$_em" "$_tm" "$_spstr"
done
IFS="$oldifs"

echo ""
echo "======================================"
echo "  RCC vs TCC vs others  --  Are-We-Fast-Yet suite"
echo "======================================"

run_bench_awfy "RCC" "$RCC" "-std=c99" "bench/awfy_rcc"
run_bench_awfy "RCC -O1" "$RCC" "-O1 -std=c99" "bench/awfy_rcc_o1"
run_bench_awfy "RCC -O2" "$RCC" "-O2 -std=c99" "bench/awfy_rcc_o2"
if [ -n "$TCC" ]; then
    run_bench_awfy "TCC" "$TCC" "-std=c99" "bench/awfy_tcc" || true
fi
if [ -n "$SLIMCC" ]; then
   run_bench_awfy "SLIMCC" "$SLIMCC" "-std=c99" "bench/awfy_slimcc" || true
fi
if [ -n "$XCC" ]; then
   run_bench_awfy "XCC" "$XCC" "-std=c99" "bench/awfy_xcc" || true
fi
if [ -n "$KEFIR" ]; then
   run_bench_awfy "KEFIR" "$KEFIR" "-std=c99" "bench/awfy_kefir" || true
   run_bench_awfy "KEFIR -O1" "$KEFIR" "-O1 -std=c99" "bench/awfy_kefir_o1" || true
fi
if [ -n "$SCC" ]; then
   run_bench_awfy "SCC" "$SCC" "-std=c99" "bench/awfy_scc" || true
fi
if [ -n "$LACC" ]; then
   run_bench_awfy "LACC" "$LACC" "-std=c99" "bench/awfy_lacc" || true
fi
if [ -n "$CPROC" ]; then
   run_bench_awfy "CPROC" "$CPROC" "-std=c99" "bench/awfy_cproc" || true
fi
if [ -n "$ANTCC" ]; then
   run_bench_awfy "ANTCC" "$ANTCC" "-std=c99" "bench/awfy_antcc" || true
fi
if [ -n "$CAKE_CC" ]; then
   run_bench_awfy "CAKE" "$CAKE_CC" "-std=c99" "bench/awfy_cake" || true
fi
if [ -n "$CCC" ]; then
   run_bench_awfy "CCC" "$CCC" "-std=c99" "bench/awfy_ccc" || true
fi
if [ -n "$BCC" ]; then
   run_bench_awfy "BCC" "$BCC" "-std=c99" "bench/awfy_bcc" || true
fi
run_bench_awfy "GCC -O0" "$GCC" "-O0 -w -std=c99 -Wno-error=incompatible-pointer-types" "bench/awfy_gcc" || true
run_bench_awfy "GCC -O2" "$GCC" "-O2 -w -std=c99 -Wno-error=incompatible-pointer-types" "bench/awfy_gcc_o2" || true
if [ -n "$CLANG" ]; then
   run_bench_awfy "Clang -O0" "$CLANG" "-O0 -w -std=c99 -Wno-error=incompatible-pointer-types" "bench/awfy_clang" || true
   run_bench_awfy "Clang -O2" "$CLANG" "-O2 -w -std=c99 -Wno-error=incompatible-pointer-types" "bench/awfy_clang_o2" || true
fi

echo ""
echo "============================================="
echo "        AWFY SCOREBOARD  (14-benchmark suite)"
echo "============================================="
printf "%-30s %10s %10s %10s %8s\n" "Compiler " "Compile" "Execute" "Total" "Spread"
printf "%-30s %10s %10s %10s %8s\n" "---------" "-------" "-------" "-----" "------"
oldifs="$IFS"
IFS='|'
for _c in $list_awfy; do
	[ -z "$_c" ] && continue
	_vname="AWFY_$(echo "$_c" | tr ' -' '__')"
	eval "_cm=\${${_vname}_COMPILE:-}"
	eval "_em=\${${_vname}_EXEC:-}"
	eval "_tm=\${${_vname}_TOTAL:-}"
	eval "_cp=\${${_vname}_COMPILE_PCT:-0}"
	eval "_ep=\${${_vname}_EXEC_PCT:-0}"
	[ -z "$_cm" ] && continue
        # shellcheck disable=SC2154
	_sp=$_cp
	[ "$_ep" -gt "$_sp" ] && _sp=$_ep
	_spstr="${_sp}%"
	[ "$_sp" -ge 20 ] && _spstr="${_sp}%!"
	printf "%-30s %8s ms %8s ms %8s ms %8s\n" "$_c" "$_cm" "$_em" "$_tm" "$_spstr"
done
IFS="$oldifs"

# ---- Large file compile-only benchmark ----
if [ -f "$LARGE_SRC" ]; then
    echo ""
    echo "============================================="
    echo "     LARGE FILE (sqlite3.c)"
    echo "============================================="
    printf "%-30s %10s %8s\n" "Compiler" "Compile (ms)" "Spread"
    printf "%-30s %10s %8s\n" "--------" "-----------" "------"

    large_results=""
    nl='
'
    _compile_large() {
	_label="$1"
	shift
	printf "%-30s " "$_label"
	_rc=0
	# shellcheck disable=SC2086
	_cbest=$(time_ms $PIN "$@" 2>/dev/null) || _rc=$?
	if [ "$_rc" -ne 0 ] || [ -z "$_cbest" ]; then
	    printf "    FAIL\n"
	    return
	fi
	_cworst=$_cbest
	_j=1
	while [ "$_j" -lt "$LARGE_RUNS" ]; do
	    _j=$((_j + 1))
	    # shellcheck disable=SC2086
	    _cm=$(time_ms $PIN "$@" 2>/dev/null) || true
	    if [ -n "$_cm" ]; then
		[ "$_cm" -lt "$_cbest" ] && _cbest=$_cm
		[ "$_cm" -gt "$_cworst" ] && _cworst=$_cm
	    fi
	done
	_pct=0
	[ "$_cbest" -gt 0 ] && _pct=$(( (_cworst - _cbest) * 100 / _cbest ))
	printf "%8s ms %7s%%\n" "$_cbest" "$_pct"
	large_results="$large_results$(printf '| %-9s | %12s | %6s |' "$_label" "${_cbest} ms" "${_pct}%")$nl"
    }

    # shellcheck disable=SC2086
    _compile_large "RCC" "$RCC" $LARGE_CFLAGS
    # shellcheck disable=SC2086
    _compile_large "RCC -O1" "$RCC" -O1 $LARGE_CFLAGS
    # shellcheck disable=SC2086
    _compile_large "RCC -O2" "$RCC" -O2 $LARGE_CFLAGS
    if [ -n "$TCC" ]; then
	# TCC defines __GNUC__ but doesn't support __uint128_t casts on ARM64
        # shellcheck disable=SC2086
	_compile_large "TCC" "$TCC" -DSQLITE_DISABLE_INTRINSIC $LARGE_CFLAGS
    fi
    if [ -n "$SLIMCC" ]; then
	# SLIMCC lacks __atomic_store_n; expected to FAIL
        # shellcheck disable=SC2086
	_compile_large "SLIMCC" "$SLIMCC" -DSQLITE_THREADSAFE=0 -D"__atomic_store_n(x,y,z)" -D"__atomic_load_n(x,z)" $LARGE_CFLAGS
    fi
    if [ -n "$XCC" ]; then
        # shellcheck disable=SC2086
	_compile_large "XCC" "$XCC" $LARGE_CFLAGS
    fi
    if [ -n "$KEFIR" ]; then
        # shellcheck disable=SC2086
	_compile_large "KEFIR" "$KEFIR" $LARGE_CFLAGS
        # shellcheck disable=SC2086
	_compile_large "KEFIR -O1" "$KEFIR" -O1 $LARGE_CFLAGS
    fi
    if [ -n "$CPROC" ]; then
        # shellcheck disable=SC2086
	_compile_large "CPROC" "$CPROC" $LARGE_CFLAGS
    fi
    if [ -n "$SCC" ]; then
        # shellcheck disable=SC2086
	_compile_large "SCC" "$SCC" $LARGE_CFLAGS
    fi
    if [ -n "$LACC" ]; then
        # shellcheck disable=SC2086
	_compile_large "LACC" "$LACC" $LARGE_CFLAGS
    fi
    if [ -n "$ANTCC" ]; then
        # shellcheck disable=SC2086
	_compile_large "ANTCC" "$ANTCC" $LARGE_CFLAGS
    fi
    if [ -n "$CCC" ]; then
        # shellcheck disable=SC2086
	_compile_large "CCC" "$CCC" $LARGE_CFLAGS
    fi
    if [ -n "$BCC" ]; then
        # shellcheck disable=SC2086
	_compile_large "BCC" "$BCC" $LARGE_CFLAGS
    fi
    # shellcheck disable=SC2086
    _compile_large "GCC -O0" "$GCC" -O0 $LARGE_CFLAGS
        # shellcheck disable=SC2086
    _compile_large "GCC -O2" "$GCC" -O2 $LARGE_CFLAGS
    if [ -n "$CLANG" ]; then
        # shellcheck disable=SC2086
	_compile_large "Clang -O0" "$CLANG" -O0 $LARGE_CFLAGS
        # shellcheck disable=SC2086
	_compile_large "Clang -O2" "$CLANG" -O2 $LARGE_CFLAGS
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
	printf "| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |\n"
	printf "| :-------- | -----------: | -----------: | ---------: | -----: |\n"
IFS='|'
for _c in $list_c; do
	[ -z "$_c" ] && continue
	_vname="$(echo "$_c" | tr ' -' '__')"
	eval "_cm=\${${_vname}_COMPILE:-}"
	eval "_em=\${${_vname}_EXEC:-}"
	eval "_tm=\${${_vname}_TOTAL:-}"
	eval "_cp=\${${_vname}_COMPILE_PCT:-0}"
	eval "_ep=\${${_vname}_EXEC_PCT:-0}"
	[ -z "$_cm" ] && continue
	_sp=$_cp
	[ "$_ep" -gt "$_sp" ] && _sp=$_ep
	printf "| %-9s | %12s | %12s | %10s | %5s%% |\n" "$_c" "$_cm" "$_em" "$_tm" "$_sp"
done
IFS="$oldifs"
printf "\n## Are-We-Fast-Yet Suite (14 benchmarks)\n\n"
printf "| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |\n"
printf "| :-------- | -----------: | -----------: | ---------: | -----: |\n"
IFS='|'
for _c in $list_awfy; do
	[ -z "$_c" ] && continue
	_vname="AWFY_$(echo "$_c" | tr ' -' '__')"
	eval "_cm=\${${_vname}_COMPILE:-}"
	eval "_em=\${${_vname}_EXEC:-}"
	eval "_tm=\${${_vname}_TOTAL:-}"
	eval "_cp=\${${_vname}_COMPILE_PCT:-0}"
	eval "_ep=\${${_vname}_EXEC_PCT:-0}"
	[ -z "$_cm" ] && continue
	_sp=$_cp
	[ "$_ep" -gt "$_sp" ] && _sp=$_ep
	printf "| %-9s | %12s | %12s | %10s | %5s%% |\n" "$_c" "$_cm" "$_em" "$_tm" "$_sp"
done
IFS="$oldifs"
printf "\n## RCC Substep Timing\n\n"
printf '```\n'
printf "RCC:\n"
printf '%s\n' "$rcc_time" | sed 's|[^ ]*/||g'
printf "\nRCC -O1:\n"
printf '%s\n' "$rcc_o1_time" | sed 's|[^ ]*/||g'
printf "\nRCC -O2:\n"
printf '%s\n' "$rcc_o2_time" | sed 's|[^ ]*/||g'
printf '```\n'
if [ -n "${rcc_large_time:-}" ]; then
	printf "\n## RCC Substep Timing -- sqlite3.c\n\n"
	printf '```\n'
	printf "RCC:\n"
	printf '%s\n' "$rcc_large_time" | sed 's|[^ ]*/||g'
	printf "\nRCC -O1:\n"
	printf '%s\n' "$rcc_large_o1_time" | sed 's|[^ ]*/||g'
	printf "\nRCC -O2:\n"
	printf '%s\n' "$rcc_large_o2_time" | sed 's|[^ ]*/||g'
	printf '```\n'
fi
if [ -n "${large_results:-}" ]; then
	printf "\n## Large File Compile-Only (sqlite3.c)\n\n"
	printf "| Compiler  | Compile (ms) | Spread |\n"
	printf "| :-------- | -----------: | -----: |\n"
	printf '%s' "$large_results"
fi
} > "$REPORT"
if command -v prettier >/dev/null 2>&1; then
	prettier --write "$REPORT" >/dev/null 2>&1 || true
fi
printf "Report: %s\n" "$REPORT"

echo ""
echo "ALL DONE"
