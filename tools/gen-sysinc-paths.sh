#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Generate src/sysinc_paths.h: a platform guard followed by
# get-sysinc-paths.sh's sys_include_paths[] array. Only touches <out> if
# the generated content actually changed, so it doesn't spuriously
# retrigger every object's rebuild.
# Usage: gen-sysinc-paths.sh <CC> <ARM64_SYSROOT> <out>

cc="$1"
sysroot="$2"
out="$3"

tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT

./tools/platform-guard.sh sysinc_paths.h "$cc" > "$tmp"
if [ "$cc" = "aarch64-linux-gnu-gcc" ] || [ -n "$sysroot" ]; then
    ./tools/get-sysinc-paths.sh "$cc --sysroot=$sysroot" >> "$tmp"
else
    ./tools/get-sysinc-paths.sh "$cc" >> "$tmp"
fi

if [ -f "$out" ] && cmp -s "$tmp" "$out"; then rm -f "$tmp"; else mv "$tmp" "$out"; fi
