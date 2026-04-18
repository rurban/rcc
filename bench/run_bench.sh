#!/bin/sh
# RCC vs TCC vs GCC benchmark (Unix version of run_bench.ps1)
set -e

BENCHDIR="$(cd "$(dirname "$0")" && pwd)"
SRC="$BENCHDIR/bench.c"
RCC="${1:-./rcc}"
TCC="${TCC:-tcc}"
GCC="${GCC:-gcc}"

RCC_EXE="$BENCHDIR/bench_rcc"
TCC_EXE="$BENCHDIR/bench_tcc"
GCC_EXE="$BENCHDIR/bench_gcc"
GCC_O2_EXE="$BENCHDIR/bench_gcc_o2"

RUNS=3

cleanup() {
	rm -f "$RCC_EXE" "$TCC_EXE" "$GCC_EXE" "$GCC_O2_EXE"
}
trap cleanup EXIT

# time_ms: prints elapsed ms for a command
# Usage: elapsed=$(time_ms cmd args...)
time_ms() {
	# Use date +%s%N if available (GNU), else fall back to seconds
	if date +%s%N >/dev/null 2>&1; then
		_start=$(date +%s%N)
		"$@"
		_rc=$?
		_end=$(date +%s%N)
		echo $(((_end - _start) / 1000000))
		return $_rc
	else
		_start=$(date +%s)
		"$@"
		_rc=$?
		_end=$(date +%s)
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

	# Compile
	_compile_ms=$(time_ms $_compiler $_args 2>/dev/null) || true
	if [ ! -x "$_exe" ]; then
		printf "  COMPILE FAILED\n"
		return 1
	fi
	printf "  Compile : %6s ms\n" "$_compile_ms"

	# Execute best of N
	_best=999999999
	_output=""
	_i=0
	while [ $_i -lt $RUNS ]; do
		_output=$("$_exe")
		_exec_ms=$(time_ms "$_exe" 2>/dev/null) || true
		if [ "$_exec_ms" -lt "$_best" ] 2>/dev/null; then
			_best=$_exec_ms
		fi
		_i=$((_i + 1))
	done
	printf "  Execute : %6s ms  (best of %d)\n" "$_best" "$RUNS"
	printf "  Total   : %6s ms\n" $((_compile_ms + _best))

	# Store for scoreboard
	eval "${_label%%[( ]*}_COMPILE=$_compile_ms"
	eval "${_label%%[( ]*}_EXEC=$_best"
	eval "${_label%%[( ]*}_TOTAL=$((_compile_ms + _best))"
	eval "${_label%%[( ]*}_OUTPUT='$_output'"
	rm -f "$_exe"
	return 0
}

echo ""
echo "============================================="
echo "  RCC vs TCC vs GCC  --  Benchmark Battle"
echo "============================================="

run_bench "RCC" "$RCC" "$SRC -o $RCC_EXE" "$RCC_EXE" || true
run_bench "TCC" "$TCC" "$SRC -o $TCC_EXE" "$TCC_EXE" || true
run_bench "GCC0" "$GCC" "-O0 $SRC -o $GCC_EXE -lm" "$GCC_EXE" || true
run_bench "GCCO2" "$GCC" "-O2 $SRC -o $GCC_O2_EXE -lm" "$GCC_O2_EXE" || true

echo ""
echo "============================================="
echo "               SCOREBOARD"
echo "============================================="
printf "%-25s %10s %10s %10s\n" "Compiler" "Compile" "Execute" "Total"
printf "%-25s %10s %10s %10s\n" "--------" "-------" "-------" "-----"
for _c in RCC TCC GCC0 GCCO2; do
	eval "_cm=\${${_c}_COMPILE:-}"
	eval "_em=\${${_c}_EXEC:-}"
	eval "_tm=\${${_c}_TOTAL:-}"
	[ -z "$_cm" ] && continue
	printf "%-25s %8s ms %8s ms %8s ms\n" "$_c" "$_cm" "$_em" "$_tm"
done

echo ""
echo "ALL DONE"
