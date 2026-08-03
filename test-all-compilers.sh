#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Run the full test suite (tcc, units, compliance, ctest, torture) against
# rcc and every reference compiler, then regenerate the "## Test Results"
# table in README.md from the resulting test-<suite>-linux_<compiler>.summary
# files that run_tests writes for each suite.
#
# Usage: ./test-all-compilers.sh [compiler...]
#   With no args: rcc gcc ccc clang tcc kefir slimcc (whichever are found).
#   With args: only run/report the named compilers, e.g.
#     ./test-all-compilers.sh gcc clang

set -e
cd "$(dirname "$0")" || exit 1

README="README.md"

RCC="./rcc"

# resolve_bin NAME: prints the binary to hand to run_tests for command NAME.
# run_tests realpath()-resolves whatever path it's given before invoking it.
# ccache dispatches to the real compiler by inspecting its own argv[0] (via
# a symlink named gcc/clang/etc.); collapsing /usr/lib64/ccache/clang to its
# realpath (/usr/bin/ccache) loses that name and ccache fails almost every
# invocation. So when command -v resolves to a ccache shim, we hand back the
# bare command name instead — realpath() on a nonexistent relative path
# fails, run_tests keeps the literal name, and PATH lookup at exec time
# preserves the argv[0] ccache needs.
resolve_bin() {
	p="$(command -v "$1" 2>/dev/null || true)"
	[ -z "$p" ] && return
	target="$(readlink -f "$p" 2>/dev/null || echo "$p")"
	if [ "$(basename "$target")" = "ccache" ]; then
		echo "$1"
	else
		echo "$p"
	fi
}

GCC="$(resolve_bin gcc)"
[ -z "$GCC" ] && GCC="gcc"
CLANG="$(resolve_bin clang)"
TCC="$(resolve_bin tcc)"
SLIMCC="$(resolve_bin slimcc)"
if [ -z "$SLIMCC" ] && [ -x ../slimcc/slimcc ]; then
	SLIMCC="../slimcc/slimcc"
fi
XCC="$(which xcc 2>/dev/null || true)"
if [ -z "$XCC" ] && [ -e "bench/../../xcc/xcc" ]; then
   XCC="../xcc/xcc"
fi
KEFIR="$(resolve_bin kefir)"
if [ -z "$KEFIR" ] && [ -x /opt/kefir/bin/kefir ]; then
	KEFIR="/opt/kefir/bin/kefir"
fi
CCC="$(resolve_bin ccc)"
if [ -z "$CCC" ] && [ -x ../claudes-c-compiler/target/release/ccc ]; then
	CCC="../claudes-c-compiler/target/release/ccc"
fi
CPROC="$(resolve_bin cproc)"
if [ -z "$CPROC" ] && [ -x ../cproc/cproc ]; then
	CPROC="../cproc/cproc"
fi
SCC="$(resolve_bin scc)"
if [ -z "$SCC" ] && [ -x ../scc/bin/scc ]; then
	SCC="../scc/bin/scc"
fi
LACC="$(resolve_bin lacc)"
if [ -z "$LACC" ] && [ -x ../lacc/bin/lacc ]; then
	LACC="../lacc/bin/lacc"
fi
ANTCC="$(resolve_bin antcc)"
if [ -z "$ANTCC" ] && [ -x ../antcc/antcc ]; then
	ANTCC="../antcc/antcc"
fi

echo "Building rcc + run_tests..."
make -s rcc run_tests

# name:binary:suffix triples, in README row order. suffix is the
# "_<compiler-basename>" run_tests appends to report filenames for
# any binary whose basename doesn't contain "rcc" (see run_tests.c).
ROW_NAMES="rcc gcc ccc clang tcc kefir antcc slimcc lacc scc xcc cproc"
bin_for() {
	case "$1" in
	rcc) echo "$RCC" ;;
	gcc) echo "$GCC" ;;
	ccc) echo "$CCC" ;;
	clang) echo "$CLANG" ;;
	tcc) echo "$TCC" ;;
	kefir) echo "$KEFIR" ;;
	slimcc) echo "$SLIMCC" ;;
	xcc) echo "$XCC" ;;
	cproc) echo "$CPROC" ;;
	lacc) echo "$LACC" ;;
	scc) echo "$SCC" ;;
	antcc) echo "$ANTCC" ;;
	esac
}
suffix_for() {
	[ "$1" = "rcc" ] && echo "linux" || echo "linux_$1"
}

ONLY=""
if [ $# -gt 0 ]; then
	ONLY=" $* "
fi

for name in $ROW_NAMES; do
	if [ -n "$ONLY" ]; then
		case "$ONLY" in *" $name "*) ;; *) continue ;; esac
	fi
	bin="$(bin_for "$name")"
	if [ -z "$bin" ]; then
		echo "=== $name: SKIP (not found) ==="
		continue
	fi
	echo "=== $name ($bin) ==="
	./run_tests "$bin" --all --parallel >"/tmp/test-all-$name.log" 2>&1 || true
	tail -3 "/tmp/test-all-$name.log"
done

# ---- Regenerate the Test Results table from all suites' .summary files ----
# Aggregates test-<suite>-<suffix>.summary for suite in $SUITES; the
# compile/error/runtime breakdown in Notes comes from torture alone, since
# it's the only suite that tracks that granularity (dg-error/dg-warning
# tests count as their own "e" bucket, separate from "c" plain compile
# failures — catching an expected error wrong is not the same bug class).
SUITES="tcc units compliance ctest torture"
TABLE=$(
	printf '| Compiler | Passed | Failed | Skipped | Notes                  |\n'
	printf '| -------- | ------ | ------ | ------- | ---------------------- |\n'
	for name in $ROW_NAMES; do
		suffix="$(suffix_for "$name")"
		AGG_TOTAL=0 AGG_PASS=0 AGG_FAIL=0 AGG_SKIP=0
		FAIL_COMPILE=0 FAIL_RUNTIME=0 ERROR_FAIL=0
		found=0
		for suite in $SUITES; do
			summary="test-${suite}-${suffix}.summary"
			[ -f "$summary" ] || continue
			found=1
			TOTAL=0 PASS=0 FAIL=0 SKIP=0
			# shellcheck disable=SC1090
			. "./$summary"
			AGG_TOTAL=$((AGG_TOTAL + TOTAL))
			AGG_PASS=$((AGG_PASS + PASS))
			AGG_FAIL=$((AGG_FAIL + FAIL))
			AGG_SKIP=$((AGG_SKIP + SKIP))
		done
		if [ "$found" -eq 0 ]; then
			printf '| %-8s | %-6s | %-6s | %-7s | %-22s |\n' "$name" "?" "?" "?" "not run"
			continue
		fi
		if [ "$AGG_FAIL" -eq 0 ]; then
			notes="100% pass rate"
		else
			denom=$((AGG_TOTAL - AGG_SKIP))
			pct=0
			[ "$denom" -gt 0 ] && pct=$((AGG_PASS * 100 / denom))
			parts=""
			[ "$FAIL_COMPILE" -gt 0 ] && parts="${parts}${FAIL_COMPILE}c/"
			[ "$ERROR_FAIL" -gt 0 ] && parts="${parts}${ERROR_FAIL}e/"
			[ "$FAIL_RUNTIME" -gt 0 ] && parts="${parts}${FAIL_RUNTIME}r/"
			parts="${parts%/}"
			if [ -n "$parts" ]; then
				notes="${pct}%, ${parts} failures"
			else
				notes="${pct}% pass rate"
			fi
		fi
		printf '| %-8s | %-6s | %-6s | %-7s | %-22s |\n' "$name" "$AGG_PASS" "$AGG_FAIL" "$AGG_SKIP" "$notes"
	done
)

awk -v table="$TABLE" '
	/<!-- TEST_RESULTS_TABLE_START -->/ { print; print ""; print table; skip = 1; next }
	/<!-- TEST_RESULTS_TABLE_END -->/ { skip = 0; print ""; print; next }
	!skip { print }
' "$README" >"$README.tmp" && mv "$README.tmp" "$README"

echo ""
echo "README.md Test Results table updated."
printf '%s\n' "$TABLE"
