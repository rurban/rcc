#!/bin/sh
set -eu

force_push=
force_plain=
last_run=
while [ $# -gt 0 ]; do
    case "$1" in
        -ff) force_push=1; force_plain=1; shift ;;
        -f) force_push=1; shift ;;
        -l|--last) last_run=1; shift ;;
        --) shift; break ;;
        -*) echo "Usage: $0 [-f|-ff] [-l|--last]" >&2; exit 1 ;;
        *) break ;;
    esac
done

if [ -n "$force_plain" ]; then
    force_flag=--force
else
    force_flag=--force-with-lease
fi

branch=$(git rev-parse --abbrev-ref HEAD)
if [ "$branch" = "main" ]; then
    echo "Error: refusing to run on main branch" >&2
    exit 1
fi

if [ -n "$last_run" ]; then
    echo "Finding last CI run for branch '$branch'..."
    run_id=$(gh run list --branch "$branch" --limit 1 --json databaseId --jq '.[0].databaseId')
    if [ -z "$run_id" ] || [ "$run_id" = "null" ]; then
        echo "Error: no CI run found for branch '$branch'" >&2
        exit 1
    fi
    echo "Using CI run $run_id..."
else
    if [ -n "$force_push" ]; then
        echo "Force-pushing branch '$branch' to trigger CI..."
        git push origin "$branch" "$force_flag"
    else
        echo "Pushing branch '$branch' to trigger CI..."
        git push origin "$branch"
    fi

    echo "Waiting for CI run to appear..."
    sleep 5

    run_id=$(gh run list --branch "$branch" --limit 1 --json databaseId --jq '.[0].databaseId')
    if [ -z "$run_id" ]; then
        echo "Error: could not find CI run for branch '$branch'" >&2
        exit 1
    fi

    echo "Watching CI run $run_id..."
    gh run watch --exit-status "$run_id" || {
        rc=$?
        echo "Warning: CI run $run_id failed or was cancelled (exit $rc)" >&2
    }
fi

echo "Downloading artifacts..."
downloaded=
m_files="bench/bench_report_darwin.md test/tcc_test_arm64.md test_report_arm64.md"
w_files="bench/bench_report_mingw.md test_report_mingw.md test/tcc_test_mingw.md"
l_files="test/tcc_test_linux.md test/torture_report_linux.log test_report_linux.md"
b_files="test_report_FreeBSD.md test_report_NetBSD.md test_report_OpenBSD.md"
dirs="linux-reports-clang linux-reports-gcc macos-reports windows-reports freebsd-reports netbsd-reports openbsd-reports"
# gh refuses to extract into an existing directory ("file exists"), so every
# artifact dir (incl. o2-report, which is never synced) must be gone first.
# shellcheck disable=SC2086
rm -rf $dirs o2-report
if gh run download "$run_id" -D .; then
    downloaded=yes
else
    echo "  -> gh run download failed (see error above)" >&2
fi

if [ -z "$downloaded" ]; then
    echo "No artifacts were downloaded, nothing to do."
    exit 0
fi
# linux-reports-gcc is synced last so its files win over clang's (same names).
for d in $dirs; do
    [ -d "$d" ] || continue
    rsync -av "$d"/ .
done

echo "Staging downloaded report files..."
# shellcheck disable=SC2086
git add $m_files $w_files $l_files $b_files 2>/dev/null || true
# shellcheck disable=SC2086
rm -rf $dirs o2-report || true

if git diff --cached --quiet; then
    echo "No report files changed, nothing to commit."
    exit 0
fi

echo "Amending commit (pre-commit hooks will reformat files)..."
if ! git commit --amend --no-edit; then
    echo "Note: commit amended but hooks may have modified files." >&2
fi

if git diff --quiet; then
    echo "No hook changes to stage."
else
    echo "Staging formatting changes from hooks..."
    git add -u
    echo "Amending again to include hook changes..."
    git commit --amend --no-edit
fi

echo "Force-pushing amended commit..."
git push origin "$branch" "$force_flag"

echo "Done."
