#!/bin/sh
# news-check.sh -- verify the NEWS file documents a release, gnulib-style.
#
# Ported from gnulib's maint.mk `news-check` target (part of the GNU
# Coding Standards: NEWS must be updated before every release) to a
# standalone script, since this project uses a plain Makefile, not
# automake/gnulib. See gnulib's top/maint.mk `news-check`,
# `news-check-regexp` and `news-check-lines-spec`.
#
# Two modes:
#
#   test/news-check.sh
#     Lint mode (no VERSION given): checks every "* Changes in ..."
#     heading in NEWS is well-formed and that versions/dates are
#     listed newest-first. Cheap, safe to run on every commit that
#     touches NEWS (see .pre-commit-config.yaml).
#
#   test/news-check.sh VERSION
#     Release mode (gnulib's actual news-check): fails unless one of
#     the first NEWS_CHECK_LINES lines (default: 1,10) of NEWS reads
#         * Changes in VERSION (YYYY-MM-DD)
#     with today's date, and is not still tagged "[planned]". Run
#     by `make release VERSION=...` right before tagging.
set -eu

news="${NEWS_CHECK_FILE:-NEWS}"
lines="${NEWS_CHECK_LINES:-1,10}"

if [ ! -f "$news" ]; then
    echo "$news: not found" >&2
    exit 1
fi

heading_re='^\* Changes in v?[0-9][0-9.]*[0-9] \(([0-9]{4}-[0-9]{2}-[0-9]{2}|\?\?\?\?-\?\?-\?\?)\)'

if [ $# -eq 0 ]; then
    # Lint mode: every heading well-formed, versions newest-first.
    headings=$(grep -nE '^\* Changes in ' "$news" || true)
    if [ -z "$headings" ]; then
        echo "$news: no '* Changes in VERSION (DATE)' headings found" >&2
        exit 1
    fi
    bad=$(printf '%s\n' "$headings" | grep -vE ":${heading_re#^}" || true)
    if [ -n "$bad" ]; then
        echo "$news: malformed heading(s), expected '* Changes in X.Y.Z (YYYY-MM-DD)':" >&2
        printf '%s\n' "$bad" >&2
        exit 1
    fi
    versions=$(printf '%s\n' "$headings" | sed -E 's/^[0-9]+:\* Changes in v?([0-9.]+) .*/\1/')
    sorted=$(printf '%s\n' "$versions" | sort -t. -k1,1nr -k2,2nr -k3,3nr)
    if [ "$versions" != "$sorted" ]; then
        echo "$news: version headings are not newest-first:" >&2
        printf '%s\n' "$versions" >&2
        exit 1
    fi
    echo "$news: OK ($(printf '%s\n' "$versions" | wc -l | tr -d ' ') release(s), newest-first)"
    exit 0
fi

# Release mode.
version="${1#v}"
version_re=$(printf '%s' "$version" | sed 's/\./\\./g')
today="$(date +%Y-%m-%d)"

match=$(sed -n "${lines}p" "$news" | grep -E "^\* Changes in v?${version_re} \(${today}\)" || true)

if [ -z "$match" ]; then
    echo "$news: no entry '* Changes in $version ($today)' in the first" \
         "$lines lines" >&2
    echo "  NEWS must be updated with today's date before releasing." >&2
    exit 1
fi

case "$match" in
    *'[planned]'*)
        echo "$news: entry for $version is still marked [planned]:" >&2
        echo "  $match" >&2
        exit 1
        ;;
esac

echo "$news: OK ($match)"
