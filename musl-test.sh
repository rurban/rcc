#!/bin/sh
# Build and run the full test suite against musl libc.
# Pre-requisites: dnf install musl-gcc musl-devel musl-libc-static
# Usage: ./musl-test.sh [test-name]
set -e

if [ -n "${1:-}" ]; then
    ./run_tests ./musl-cross.sh "$@"
else
    echo "==> Running full test suite against musl libc..."
    echo ""
    ./run_tests ./musl-cross.sh --all --parallel
fi
