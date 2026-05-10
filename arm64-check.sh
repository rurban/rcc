#!/bin/sh
# Cross-build ARM64 rcc and run the TCC test suite against it.
set -e

make leanclean
make -s CC=aarch64-linux-gnu-gcc
timeout 3m ./run_tcc_suite.sh ./rcc-arm64
