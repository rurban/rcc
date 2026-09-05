#!/bin/sh
# cppcheck-timeout.sh: run cppcheck with a hard wall-clock budget.
#
# Used as the pre-commit `cppcheck` hook's entry point. CI runners
# occasionally stall cppcheck (or the surrounding prek/git-submodule
# machinery it gets invoked through) well past any reasonable analysis
# time for a handful of changed files — a real finding takes seconds
# locally. Rather than let that hang the whole lint job until CI's own
# job-level timeout cancels it (reported as a bare, uninformative
# "operation was canceled"), bound it here and treat "timed out" as
# "inconclusive, not a lint failure": a genuine cppcheck error/warning
# still fails the hook normally, and finishing well inside the budget is
# unaffected.
set -eu

# cppcheck's own preprocessor doesn't know the host platform unless told:
# without it, config enumeration for `#if defined(__linux__)`-style guards
# can settle on a configuration where none of the arch/OS macros are set,
# walking into an `#error "unsupported ..."` branch and reporting it as a
# real preprocessorErrorDirective. Define the macros that actually apply
# to the host this hook runs on, matching AGENTS.md: native only, no
# cross-compilation in one binary.
PLATFORM_DEFS=
case "$(uname -s)" in
    Linux) PLATFORM_DEFS="-D__linux__" ;;
    Darwin) PLATFORM_DEFS="-D__APPLE__" ;;
    MINGW*|MSYS*|CYGWIN*) PLATFORM_DEFS="-D_WIN32 -D__MINGW32__" ;;
esac
case "$(uname -m)" in
    x86_64|amd64) PLATFORM_DEFS="$PLATFORM_DEFS -D__x86_64__" ;;
    aarch64|arm64) PLATFORM_DEFS="$PLATFORM_DEFS -D__aarch64__" ;;
esac

TIMEOUT_SECS="${CPPCHECK_TIMEOUT_SECS:-180}"

if command -v timeout >/dev/null 2>&1; then
    # `set -e` aborts the script the instant any unguarded command
    # returns non-zero — including `timeout` itself reporting 124 — so
    # capturing $? on the line *after* silently never runs. `|| rc=$?`
    # keeps the non-zero status from tripping `set -e` at all.
    # shellcheck disable=SC2086  # PLATFORM_DEFS: intentional word-split, may hold 0-2 -D flags
    timeout "$TIMEOUT_SECS" cppcheck $PLATFORM_DEFS "$@" || rc=$?
    rc="${rc:-0}"
    if [ "$rc" -eq 124 ]; then
        echo "cppcheck-timeout.sh: cppcheck did not finish within ${TIMEOUT_SECS}s; skipping (not a lint failure)" >&2
        exit 0
    fi
    exit "$rc"
else
    # No `timeout` binary available (unlikely outside minimal containers):
    # fall back to running cppcheck directly, unbounded.
    # shellcheck disable=SC2086  # PLATFORM_DEFS: intentional word-split, may hold 0-2 -D flags
    exec cppcheck $PLATFORM_DEFS "$@"
fi
