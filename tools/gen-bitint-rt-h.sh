#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Generate src/bitint_rt.h: src/bitint_rt.c embedded as a C string literal
# (via embed-c.sh), for runtime-compiling the _BitInt helpers on demand.
# Only touches <out> if the generated content actually changed.
# Usage: gen-bitint-rt-h.sh <out>

out="$1"

tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT

printf 'static const char bitint_rt_src[] =\n' > "$tmp"
./tools/embed-c.sh src/bitint_rt.c >> "$tmp"
printf ';\n' >> "$tmp"

if [ -f "$out" ] && cmp -s "$tmp" "$out"; then rm -f "$tmp"; else mv "$tmp" "$out"; fi
