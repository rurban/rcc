#!/bin/sh
# shellcheck disable=SC2034  # counters below are read indirectly via getcount()/inc()
##
## rcc_optlevels.sh -- differential-test all of rcc's -O* optimization
## levels against each other (and optionally against a reference
## compiler) using randomly generated Csmith programs.
##
## For every generated program this:
##   1. compiles it with ../rcc/rcc at -O0, -O1, -O2 and -O3
##   2. runs each resulting binary under a timeout
##   3. uses the -O0 build (peephole optimizer disabled) as the
##      per-test baseline and diffs -O1/-O2/-O3 output + exit code
##      against it -- any divergence is an optimizer-introduced bug
##      specific to that level
##   4. optionally also builds/runs the program with $REFCC and
##      diffs the -O0 baseline against it -- a divergence there is a
##      general codegen bug (present regardless of optimization)
##
## Failing sources are kept under $WORKDIR. When REDUCE=1 (default) and
## cvise or creduce is installed, each newly found bug is additionally
## shrunk in-place to a minimal reproducer right after it is detected.
## Reductions are detached with nohup and run in the background, so the
## test loop keeps going and a reduction survives a hangup (closed terminal
## or this script exiting). Each result appears as
## $WORKDIR/$pid-reduced_<kind>_<lvl>_<num>.c once its reduction finishes.
##
## Usage: rcc_optlevels.sh [N]
##
## Env vars:
##   RCC             path to the rcc binary under test
##                   (default: sibling ../rcc/rcc next to this csmith checkout)
##   CSMITH          csmith binary                    (default: csmith)
##   CSMITH_INCLUDE  dir containing csmith.h           (autodetected)
##   REFCC           reference compiler, "" to disable (default: gcc)
##   REFCC_OPT       optimization flags for $REFCC     (default: -O2)
##   OPT_LEVELS      space-separated rcc -O flags      (default: "O0 O1 O2 O3")
##   TIMEOUT         per-run timeout in seconds         (default: 2)
##   CFLAGS          extra flags passed to every rcc invocation
##   REDUCE          1 to auto-reduce each new bug, 0 to disable (default: 1)
##   REDUCE_TOOL     cvise|creduce      (default: cvise, else creduce, else off)
##   REDUCE_TIMEOUT  wall-clock cap for one reduction, seconds (default: 180)
##   REDUCE_JOBS     parallel jobs passed to cvise's -n     (default: auto)
##   PP_CC           compiler used to preprocess before reducing (default: gcc)
##
###############################################################################

set -e

# keep a handle to the original stdout so nohup'd background reductions can
# still report completion to the console even when stdout is redirected
exec 3>&1

# resolve RCC to an absolute path: reduction runs interestingness tests from
# a scratch directory, so a relative override must survive the directory change
ORIG_PWD=$(pwd)

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/.." && pwd)

RCC=${RCC:-"$ROOT/rcc"}
case "$RCC" in
/*) ;;
*) RCC="$ORIG_PWD/$RCC" ;;
esac
CSMITH=${CSMITH:-csmith}
REFCC=${REFCC-gcc}
REFCC_OPT=${REFCC_OPT:-"-O2"}
OPT_LEVELS=${OPT_LEVELS:-"O0 O1 O2 O3"}
TIMEOUT=${TIMEOUT:-"2"}
CFLAGS=${CFLAGS:-""}
REDUCE=${REDUCE:-1}
REDUCE_TOOL=${REDUCE_TOOL:-""}
REDUCE_TIMEOUT=${REDUCE_TIMEOUT:-180}
REDUCE_JOBS=${REDUCE_JOBS:-""}
PP_CC=${PP_CC:-gcc}

if [ ! -x "$RCC" ]; then
    echo "OOPS: rcc binary not found/executable at '$RCC' (set RCC=... to override)" >&2
    exit 1
fi

if [ -z "$CSMITH_INCLUDE" ]; then
    if [ -f /usr/local/include/csmith.h ]; then
        CSMITH_INCLUDE=/usr/local/include
    elif [ -f /usr/include/csmith/csmith.h ]; then
        CSMITH_INCLUDE=/usr/include/csmith
    else
        echo "OOPS: cannot find csmith.h; set CSMITH_INCLUDE=... to override" >&2
        exit 1
    fi
fi

if [ "$REDUCE" = 1 ]; then
    if [ -z "$REDUCE_TOOL" ]; then
        if command -v cvise >/dev/null 2>&1; then
            REDUCE_TOOL=cvise
        elif command -v creduce >/dev/null 2>&1; then
            REDUCE_TOOL=creduce
        fi
    fi
    if [ -z "$REDUCE_TOOL" ] || ! command -v "$REDUCE_TOOL" >/dev/null 2>&1; then
        echo "NOTE: no cvise/creduce found; disabling automatic reduction (set REDUCE=0 to silence)" >&2
        REDUCE=0
    fi
fi

cd "$ROOT"
WORKDIR=test/csmith
mkdir -p "$WORKDIR"

N=${1:-100}
pid=$$

# per-level counters (assigned/read indirectly via inc()/getcount() eval)
npass_O0=0; npass_O1=0; npass_O2=0; npass_O3=0
ncomperr_O0=0; ncomperr_O1=0; ncomperr_O2=0; ncomperr_O3=0
ntimeout_O0=0; ntimeout_O1=0; ntimeout_O2=0; ntimeout_O3=0
nmismatch_O1=0; nmismatch_O2=0; nmismatch_O3=0
nrefmismatch=0
nreferr=0

rc=0
REDUCE_PIDS=""

echo "($pid) Running $N tests"
echo "rcc=$RCC"
echo "opt levels: $OPT_LEVELS"
echo "reference compiler: ${REFCC:-<disabled>} $REFCC_OPT"
echo "csmith include: $CSMITH_INCLUDE"
echo "timeout=${TIMEOUT}s"
echo "rcc CFLAGS=$CFLAGS"
if [ "$REDUCE" = 1 ]; then
    echo "reduce: enabled ($REDUCE_TOOL, ${REDUCE_TIMEOUT}s cap)"
else
    echo "reduce: disabled"
fi

# read one counter var by name
getcount() { eval "echo \"\$$1\""; }
# increment one counter var by name
inc() { eval "$1=\$(($(getcount "$1") + 1))"; }

# reduce a just-found failing test case to a minimal reproducer with
# cvise/creduce. The reducer is launched in the background under nohup, so
# it does not block the test loop and survives a hangup; once it finishes it
# writes $WORKDIR/$pid-reduced_<kind>_<lvl>_<num>.c plus the matching
# .sh interestingness test used to validate it.
#   $1 kind: compile | optmismatch | refmismatch
#   $2 lvl:  rcc -O level implicated (compile/optmismatch) or O0 (refmismatch)
#   $3 num:  test number
#   $4 src:  path to the original (unreduced) failing source
#   $5 sig:  distinguishing text from the compiler error (compile kind only)
reduce_case() {
    kind=$1
    lvl=$2
    num=$3
    src=$4
    sig=$5

    [ "$REDUCE" = 1 ] || return 0

    rdir="$WORKDIR/$pid-reduce-$kind-$lvl-$num"
    rm -rf "$rdir"
    mkdir -p "$rdir"

    if ! "$PP_CC" -E -I"$CSMITH_INCLUDE" "$src" >"$rdir/test.c" 2>"$rdir/pp.err"; then
        echo "  (reduce skipped: preprocessing failed, see $rdir/pp.err)"
        return 0
    fi

    itest="$rdir/interesting.sh"
    case "$kind" in
    compile)
        printf '%s\n' "$sig" >"$rdir/sig.txt"
        cat >"$itest" <<SCRIPT
#!/bin/sh
$RCC "-$lvl" $CFLAGS test.c -o t.exe -lm >err.txt 2>&1
rc=\$?
[ \$rc -eq 0 ] && exit 1
if [ -s sig.txt ]; then
    grep -qF -f sig.txt err.txt || exit 1
fi
exit 0
SCRIPT
        ;;
    optmismatch)
        cat >"$itest" <<SCRIPT
#!/bin/sh
$RCC -O0 $CFLAGS test.c -o t0.exe -lm >/dev/null 2>&1 || exit 1
$RCC "-$lvl" $CFLAGS test.c -o t1.exe -lm >/dev/null 2>&1 || exit 1
out0=\$(timeout $TIMEOUT ./t0.exe 2>&1); st0=\$?
[ \$st0 -eq 124 ] && exit 1
out1=\$(timeout $TIMEOUT ./t1.exe 2>&1); st1=\$?
[ \$st1 -eq 124 ] && exit 1
[ "\$st0" = "\$st1" ] && [ "\$out0" = "\$out1" ] && exit 1
exit 0
SCRIPT
        ;;
    refmismatch)
        cat >"$itest" <<SCRIPT
#!/bin/sh
$RCC -O0 $CFLAGS test.c -o t0.exe -lm >/dev/null 2>&1 || exit 1
$REFCC -w $REFCC_OPT test.c -o tref.exe -lm >/dev/null 2>&1 || exit 1
out0=\$(timeout $TIMEOUT ./t0.exe 2>&1); st0=\$?
[ \$st0 -eq 124 ] && exit 1
outref=\$(timeout $TIMEOUT ./tref.exe 2>&1); stref=\$?
[ \$stref -eq 124 ] && exit 1
[ "\$st0" = "\$stref" ] && [ "\$out0" = "\$outref" ] && exit 1
exit 0
SCRIPT
        ;;
    *)
        rm -rf "$rdir"
        return 0
        ;;
    esac
    chmod +x "$itest"

    if ! (cd "$rdir" && ./interesting.sh); then
        echo "  (reduce skipped: interestingness test did not confirm the bug)"
        rm -rf "$rdir"
        return 0
    fi

    reduce_jobs_opt=""
    if [ -n "$REDUCE_JOBS" ] && [ "$REDUCE_TOOL" = cvise ]; then
        reduce_jobs_opt="-n $REDUCE_JOBS"
    fi

    dest="$WORKDIR/$pid-reduced_${kind}_${lvl}_$num.c"
    destsh="$WORKDIR/$pid-reduced_${kind}_${lvl}_$num.sh"

    # Run the reducer detached via nohup so it does not block the test loop
    # and survives a hangup (closed terminal or this script exiting). The
    # runner below copies the result into $WORKDIR and removes its scratch
    # dir itself, so the parent only needs to track the pid.
    cat >"$rdir/reduce_bg.sh" <<'EOF'
#!/bin/sh
rdir=$1
tool=$2
cap=$3
jobs_opt=$4
dest=$5
destsh=$6
cd "$rdir" || exit 1
timeout "$cap" "$tool" $jobs_opt interesting.sh test.c
if [ -s test.c ]; then
    cp test.c "$dest"
    cp interesting.sh "$destsh"
    echo "  reduced case: $dest ($(wc -l <"$dest") lines)" >&3
fi
rm -rf "$rdir"
EOF
    chmod +x "$rdir/reduce_bg.sh"

    nohup "$rdir/reduce_bg.sh" "$rdir" "$REDUCE_TOOL" "$REDUCE_TIMEOUT" \
        "$reduce_jobs_opt" "$dest" "$destsh" \
        </dev/null >"$rdir/reduce.log" 2>&1 &
    bgpid=$!
    REDUCE_PIDS="$REDUCE_PIDS $bgpid"
    echo "  reducing with $REDUCE_TOOL in background (pid $bgpid, result: $dest)"
}

run_test() {
    num=$1
    src="$WORKDIR/$pid-$num.c"

    "$CSMITH" > "$src" 2>/dev/null || return 1

    baseline_out=""
    baseline_exit=""
    have_baseline=0

    for lvl in $OPT_LEVELS; do
        exe="$WORKDIR/$pid-$num-$lvl"
        out="$exe.out"
        comperr="$exe.comperr"

        # shellcheck disable=SC2086
        if ! "$RCC" "-$lvl" $CFLAGS -I"$CSMITH_INCLUDE" "$src" -o "$exe" -lm >"$comperr" 2>&1; then
            inc "ncomperr_$lvl"
            echo ""
            echo "FAIL (compile at -$lvl): test $num"
            cat "$comperr"
            cp "$src" "$WORKDIR/$pid-fail_${lvl}_compile_$num.c"
            sig=$(grep -m1 -i "error:" "$comperr" 2>/dev/null | sed 's/.*[Ee]rror: *//')
            reduce_case compile "$lvl" "$num" "$src" "$sig"
            rc=1
            rm -f "$exe" "$out" "$comperr"
            continue
        fi
        rm -f "$comperr"

        timeout "$TIMEOUT" "$exe" >"$out" 2>&1 </dev/null
        st=$?

        if [ $st -eq 124 ]; then
            inc "ntimeout_$lvl"
            rm -f "$exe" "$out"
            continue
        fi

        if [ "$lvl" = "O0" ]; then
            baseline_out=$(cat "$out")
            baseline_exit=$st
            have_baseline=1
            inc "npass_$lvl"
        elif [ $have_baseline -eq 0 ]; then
            # no O0 baseline available for this test (compile/timeout failed
            # above); nothing to diff this level against, just count it
            inc "npass_$lvl"
        elif [ "$st" != "$baseline_exit" ] || [ "$(cat "$out")" != "$baseline_out" ]; then
            eval "nmismatch_$lvl=\$((\$(getcount nmismatch_$lvl) + 1))"
            echo ""
            echo "FAIL (mismatch vs -O0): test $num, level -$lvl"
            echo "  -O0: exit=$baseline_exit out=$baseline_out"
            echo "  -$lvl: exit=$st out=$(cat "$out")"
            cp "$src" "$WORKDIR/$pid-fail_${lvl}_optmismatch_$num.c"
            reduce_case optmismatch "$lvl" "$num" "$src" ""
            rc=1
        else
            inc "npass_$lvl"
        fi

        rm -f "$exe" "$out"
    done

    if [ -n "$REFCC" ] && [ $have_baseline -eq 1 ]; then
        refexe="$WORKDIR/$pid-$num-ref"
        refout="$refexe.out"
        referr="$refexe.comperr"
        # shellcheck disable=SC2086
        if ! "$REFCC" -I"$CSMITH_INCLUDE" -w $REFCC_OPT "$src" -o "$refexe" -lm >"$referr" 2>&1; then
            nreferr=$((nreferr + 1))
            echo ""
            echo "FAIL ($REFCC compile): test $num"
            cat "$referr"
            rm -f "$refexe" "$refout" "$referr"
        else
            rm -f "$referr"
            timeout "$TIMEOUT" "$refexe" >"$refout" 2>&1 </dev/null
            refst=$?
            if [ $refst -ne 124 ]; then
                if [ "$refst" != "$baseline_exit" ] || [ "$(cat "$refout")" != "$baseline_out" ]; then
                    nrefmismatch=$((nrefmismatch + 1))
                    echo ""
                    echo "FAIL (rcc -O0 vs $REFCC): test $num"
                    echo "  rcc -O0: exit=$baseline_exit out=$baseline_out"
                    echo "  $REFCC:  exit=$refst out=$(cat "$refout")"
                    cp "$src" "$WORKDIR/$pid-fail_refmismatch_$num.c"
                    reduce_case refmismatch O0 "$num" "$src" ""
                    rc=1
                fi
            fi
            rm -f "$refexe" "$refout"
        fi
    fi

    rm -f "$src"
    printf "."
    return 0
}

i=0
while [ $i -lt "$N" ]; do
    run_test $i || true
    i=$((i + 1))
done

# wait for any still-running background reductions so their results land in
# $WORKDIR before we print the summary (nohup keeps them alive even if we
# were hung up mid-run)
if [ -n "${REDUCE_PIDS:-}" ]; then
    # shellcheck disable=SC2086
    wait $REDUCE_PIDS 2>/dev/null || true
fi

echo ""
echo "Results per optimization level (pass/comperr/timeout/mismatch-vs--O0):"
for lvl in $OPT_LEVELS; do
    printf "  -%-3s %4d pass, %4d comperr, %4d timeout" \
        "$lvl" "$(getcount "npass_$lvl")" "$(getcount "ncomperr_$lvl")" "$(getcount "ntimeout_$lvl")"
    if [ "$lvl" != "O0" ]; then
        printf ", %4d mismatch" "$(getcount "nmismatch_$lvl")"
    fi
    echo ""
done
if [ -n "$REFCC" ]; then
    echo "  vs $REFCC: $nreferr compile errors, $nrefmismatch mismatches"
fi

find "$WORKDIR"/ -name "$pid-fail_*" 2>/dev/null | grep "$pid" && echo "(failing sources kept above for reduction)"

exit $rc
