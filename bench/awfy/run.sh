#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Are-We-Fast-Yet "Richards" benchmark: rcc vs gcc GENERATED-CODE speed.
#
# Unlike bench/run_bench.sh (which times how fast each compiler itself
# compiles), this measures how fast the *compiled program* runs --
# i.e. the quality of rcc's generated code on a nontrivial, real-world
# style C program (OS process-scheduler simulation; allocation- and
# pointer-chasing-heavy).
#
# Usage: ./bench/awfy/run.sh [rcc-binary] [iterations]
set -e

cd "$(dirname "$0")" || exit 1
RCC="${1:-../../rcc}"
ITER="${2:-100}"
GCC="${GCC:-gcc}"

if [ ! -x "$RCC" ]; then
    echo "ERROR: rcc not found at '$RCC'. Build it first." >&2
    exit 1
fi

for opt in -O0 -O1 -O2; do
    printf "\n--- rcc %s ---\n" "$opt"
    "$RCC" "$opt" -std=c99 -o /tmp/awfy_richards_rcc Richards.c Benchmark.c main.c -lm
    /tmp/awfy_richards_rcc "$ITER" 1
    rm -f /tmp/awfy_richards_rcc
done

if command -v "$GCC" >/dev/null 2>&1; then
    printf "\n--- gcc -O2 (baseline) ---\n"
    "$GCC" -O2 -w -std=c99 -fpermissive -o /tmp/awfy_richards_gcc Richards.c Benchmark.c main.c -lm
    /tmp/awfy_richards_gcc "$ITER" 1
    rm -f /tmp/awfy_richards_gcc
fi
