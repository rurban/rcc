#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Are-We-Fast-Yet benchmark suite: rcc vs gcc GENERATED-CODE speed.
#
# Unlike bench/run_bench.sh (which times how fast each compiler itself
# compiles), this measures how fast the *compiled programs* run --
# i.e. the quality of rcc's generated code across 14 diverse, real-
# world-style C programs (OS process scheduling, JSON parsing, a
# constraint solver, dataflow-graph loop analysis, collision
# detection, red-black trees, N-body simulation, ...).
#
# Usage: ./bench/awfy/run.sh [rcc-binary]
set -e

cd "$(dirname "$0")" || exit 1
RCC="${1:-../../rcc}"
GCC="${GCC:-gcc}"

if [ ! -x "$RCC" ]; then
    echo "ERROR: rcc not found at '$RCC'. Build it first." >&2
    exit 1
fi

for opt in -O0 -O1 -O2; do
    printf "\n============================================\n"
    printf "  rcc %s\n" "$opt"
    printf "============================================\n"
    "$RCC" "$opt" -std=c99 -o /tmp/awfy_rcc ./*.c som/*.c -lm
    /tmp/awfy_rcc
    rm -f /tmp/awfy_rcc
done

if command -v "$GCC" >/dev/null 2>&1; then
    printf "\n============================================\n"
    printf "  gcc -O2 (baseline)\n"
    printf "============================================\n"
    "$GCC" -O2 -w -std=c99 -fpermissive -o /tmp/awfy_gcc ./*.c som/*.c -lm
    /tmp/awfy_gcc
    rm -f /tmp/awfy_gcc
fi
