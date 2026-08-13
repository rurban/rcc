#!/bin/sh
# Convert a C source file into a C string literal initializer, for
# self-hosted embedding (rcc compiles its own bitint runtime per-TU).
# Usage: embed-c.sh <file> > header-fragment
awk '{ gsub(/\\/, "\\\\"); gsub(/"/, "\\\""); printf "\"%s\\n\"\n", $0 }' "$1"
