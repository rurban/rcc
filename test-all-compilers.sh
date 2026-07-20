#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Run the full test suite against rcc and every reference compiler, then
# regenerate the "## Test Results" torture-suite table in README.md from
# the resulting test-torture-linux_<compiler>.summary / torture_report logs.
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

echo "Building rcc + run_tests..."
make -s rcc run_tests

# name:binary:suffix triples, in README row order. suffix is the
# "_<compiler-basename>" run_tests appends to report filenames for
# any binary whose basename doesn't contain "rcc" (see run_tests.c).
ROW_NAMES="rcc gcc ccc clang tcc kefir slimcc xcc"
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

# ---- Regenerate the torture-suite table from the .summary/.log files ----
TABLE=$(
	printf '| Compiler | Passed | Failed | Skipped | Notes                  |\n'
	printf '| -------- | ------ | ------ | ------- | ---------------------- |\n'
	for name in $ROW_NAMES; do
		suffix="$(suffix_for "$name")"
		summary="test-torture-${suffix}.summary"
		log="test/torture_report_${suffix}.log"
		if [ ! -f "$summary" ]; then
			printf '| %-8s | %-6s | %-6s | %-7s | %-22s |\n' "$name" "?" "?" "?" "not run"
			continue
		fi
		TOTAL=0 PASS=0 FAIL=0 FAIL_COMPILE=0 FAIL_RUNTIME=0 SKIP=0
		# shellcheck disable=SC1090
		. "./$summary"
		if [ "$FAIL" -eq 0 ]; then
			notes="100% pass rate"
		elif [ "$FAIL" -le 5 ] && [ -f "$log" ]; then
			notes="$(grep 'FAIL' "$log" | awk '{print $1}' | tr '\n' '+' | sed 's/+$//; s/+/ + /g')"
		else
			denom=$((TOTAL - SKIP))
			pct=0
			[ "$denom" -gt 0 ] && pct=$((PASS * 100 / denom))
			notes="${pct}%, ${FAIL_COMPILE}c/${FAIL_RUNTIME}r failures"
		fi
		printf '| %-8s | %-6s | %-6s | %-7s | %-22s |\n' "$name" "$PASS" "$FAIL" "$SKIP" "$notes"
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
