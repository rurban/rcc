#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Generate src/gcc_predefined.h: a platform guard followed by
# get-gcc-predefined.awk's rendering of the compiler's predefined macros.
# Only touches <out> if the generated content actually changed, so it
# doesn't spuriously retrigger every object's rebuild.
# Usage: gen-gcc-predefined.sh <CC> <out>

cc="$1"
out="$2"

dm=$(mktemp)
tmp=$(mktemp)
trap 'rm -f "$dm" "$tmp"' EXIT

"$cc" -dM -E - < /dev/null > "$dm"
./tools/platform-guard.sh gcc_predefined.h "$cc" "$dm" > "$tmp"
awk -f tools/get-gcc-predefined.awk "$dm" >> "$tmp"

if [ -f "$out" ] && cmp -s "$tmp" "$out"; then rm -f "$tmp"; else mv "$tmp" "$out"; fi
