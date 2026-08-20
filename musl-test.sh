#!/bin/sh
# Build and run the full test suite against musl libc.
# Pre-requisites (rpm): dnf install musl-gcc musl-devel musl-libc-static
# Pre-requisites (apt): apt install musl-dev musl
# Usage: ./musl-test.sh [test-name]
set -e

if [ -n "${1:-}" ]; then
    ./run_tests ./musl-cross.sh "$@"
else
    echo "==> Running full test suite against musl libc..."
    echo ""
    ./run_tests ./musl-cross.sh --all --parallel
fi
