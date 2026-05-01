#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Extract system include paths from a C compiler.
# Usage: get-sysinc-paths.sh <compiler>
# Outputs a C header fragment defining sys_include_paths[].

compiler="${1:-gcc}"

echo "static const char *sys_include_paths[] = {"

# Extract paths between "#include <...> search starts here:" and "End of search list"
$compiler -xc -E -v - </dev/null 2>&1 | \
sed -n '/#include <\.\.\.> search starts here:/,/End of search list/p' | \
sed '1d;$d' | \
while read -r line; do
    # Trim leading whitespace
    path="$(echo "$line" | sed 's/^[[:space:]]*//')"
    # Skip empty lines and lines that don't look like paths
    [ -z "$path" ] && continue
    # Check if directory actually exists
    [ -d "$path" ] || continue
    printf '    "%s",\n' "$path"
done

echo "    NULL"
echo "};"
