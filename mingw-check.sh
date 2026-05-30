#!/bin/sh
# Cross-build Windows rcc and run the TCC test suite against it.
set -e

make leanclean
make -s CC=x86_64-w64-mingw32-gcc
timeout 5m ./run_tcc_suite.sh ./rcc.exe
