#!/bin/bash
# Run test/torture/run.sh, capture stdout to test/torture_report_$PLATFORM.log.
# PLATFORM is determined inside run.sh and saved in test-torture-$PLATFORM.summary.
# Usage: ./capture.sh [run.sh-args]

cd "$(dirname "$0")/../.." || exit 1
tmpf=$(mktemp /tmp/torture_capture.XXXXXX)
./test/torture/run.sh "$@" 2>&1 | tee "$tmpf"; rc=${PIPESTATUS[0]}
plat=$(find . -maxdepth 1 -name 'test-torture-*.summary' -printf '%f\n' 2>/dev/null | head -1 | sed 's/^test-torture-//;s/\.summary$//')
if [ -n "$plat" ]; then
    mv "$tmpf" "test/torture_report_${plat}.log"
else
    echo "ERROR: no test-torture-*.summary found" >&2
    rm -f "$tmpf"
fi
exit "$rc"
