#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# -g3 (full DWARF) tests: DIEs, locations, lexical blocks, types, macros,
# debugger-visible behaviour and the line-table-only default of -g/-g2.
# Needs an rcc linked against libdwarfp (HAVE_LIBDWARF), readelf, gcc;
# skips (exit 0) otherwise.  gdb checks run only when gdb is installed.
#
# Usage: ./test/test-g3.sh [rcc-binary]      (default: ./rcc)

cd "$(dirname "$0")/.." || exit 1
RCC="${1:-./rcc}"
[ -x "$RCC" ] || { echo "ERROR: rcc not found at '$RCC'" >&2; exit 1; }
RCC="$(cd "$(dirname "$RCC")" && pwd)/$(basename "$RCC")"

skip() { echo "SKIP: -g3 tests ($1)"; exit 0; }
[ "$(uname -s)" = Linux ] && [ "$(uname -m)" = x86_64 ] || skip "needs Linux x86-64"
command -v readelf >/dev/null 2>&1 || skip "no readelf"
command -v gcc >/dev/null 2>&1 || skip "no gcc"
ldd "$RCC" 2>/dev/null | grep -q libdwarf || skip "rcc built without libdwarfp"

T="$(mktemp -d)" || exit 1
trap 'rm -rf "$T"' EXIT
cd "$T" || exit 1
PASS=0; FAIL=0
ok()   { PASS=$((PASS + 1)); echo "PASS: $1"; }
bad()  { FAIL=$((FAIL + 1)); echo "FAIL: $1"; }
check() { # name, command...
    n="$1"; shift
    if "$@" >/dev/null 2>&1; then ok "$n"; else bad "$n"; fi
}

cat > t.c <<'EOF'
#define SQUARE(x) ((x)*(x))
#define LIMIT 100
#undef LIMIT
#define LIMIT 200
typedef unsigned int u32;
struct Point { int x; int y; u32 tag; char pad[3]; unsigned b1:1; int b5:5; };
int gcount = 5;
int sq(int v) { int t = SQUARE(v); if (t > LIMIT) t = LIMIT; return t; }
int main(void) {
    struct Point p = {1, 2, 3, {0}, 1, 7};
    int i, sum = 0;
    for (i = 0; i < 10; i++) {
        int local_i = i;
        sum += local_i;
    }
    int sum2 = sq(7);
    return (p.x + sum + sum2 + gcount) == 1 + 45 + 49 + 5 ? 0 : 1;
}
EOF

"$RCC" -g3 -c t.c -o t.o 2>err.txt || { cat err.txt; bad "rcc -g3 -c"; exit 1; }
check "no libdwarf fallback warning" test ! -s err.txt
readelf --debug-dump=info t.o > info.txt 2>&1
check "subprogram sq"        grep -q 'DW_AT_name .*: sq$' info.txt
check "subprogram main"      grep -q 'DW_AT_name .*: main$' info.txt
check "structure_type Point" grep -q 'DW_AT_name .*: Point$' info.txt
check "bit-field member"     grep -q 'DW_AT_bit_size' info.txt
check "lexical block"        grep -q 'DW_TAG_lexical_block' info.txt
check "block local variable" grep -q 'DW_AT_name .*: local_i$' info.txt
check "parameter"            grep -q 'DW_TAG_formal_parameter' info.txt
check "global with DW_OP_addr" grep -q 'DW_OP_addr' info.txt
check "typedef u32"          grep -q 'DW_AT_name .*: u32$' info.txt
check "array bound"          grep -q 'DW_AT_upper_bound' info.txt
if readelf --debug-dump=macro t.o 2>/dev/null | grep -q SQUARE; then ok "macro SQUARE"; else bad "macro SQUARE"; fi
if readelf --debug-dump=macro t.o 2>/dev/null | grep -q 'undef.*LIMIT'; then ok "macro undef LIMIT"; else bad "macro undef LIMIT"; fi

# Section-relative references must be relocated (several CUs get linked).
if readelf -r t.o | grep -q 'R_X86_64_32 .*\.debug_abbrev'; then ok "abbrev reloc"; else bad "abbrev reloc"; fi
if readelf -r t.o | grep -q 'R_X86_64_32 .*\.debug_line'; then ok "stmt_list reloc"; else bad "stmt_list reloc"; fi

# -g / -g2 stay line-table only.
"$RCC" -g2 -c t.c -o t2.o 2>/dev/null
if readelf --debug-dump=info t2.o 2>/dev/null | grep -q DW_TAG_subprogram; then
    bad "-g2 is line table only"
else
    ok "-g2 is line table only"
fi

# Link + run with gcc and with rcc's own linker.
if gcc t.o -o t_gcc && ./t_gcc; then ok "gcc link, runs"; else bad "gcc link, runs"; fi
if "$RCC" -g3 t.c -o t_rcc && ./t_rcc; then ok "rcc link, runs"; else bad "rcc link, runs"; fi
if readelf --debug-dump=info t_rcc 2>/dev/null | grep -q DW_TAG_subprogram; then ok "rcc-linked exe keeps DIEs"; else bad "rcc-linked exe keeps DIEs"; fi

if command -v gdb >/dev/null 2>&1; then
    gdb -batch -ex 'break sq' -ex run -ex 'info args' -ex 'up' -ex 'info locals' \
        -ex 'p gcount' -ex 'ptype struct Point' -ex 'macro expand SQUARE(3)' t_gcc > gdb1.txt 2>&1
    if grep -q '^v = 7' gdb1.txt; then ok "gdb: parameter value"; else bad "gdb: parameter value"; fi
    if grep -q 'int b5 : 5;' gdb1.txt; then ok "gdb: ptype bit-field"; else bad "gdb: ptype bit-field"; fi
    if grep -q "^[$]1 = 5" gdb1.txt; then ok "gdb: global value"; else bad "gdb: global value"; fi
    if grep -q '((3)\*(3))' gdb1.txt; then ok "gdb: macro expand"; else bad "gdb: macro expand"; fi
    gdb -batch -ex 'break 15' -ex run -ex 'info locals' -ex 'p p' t_gcc > gdb2.txt 2>&1
    if grep -q 'local_i = ' gdb2.txt; then ok "gdb: block local"; else bad "gdb: block local"; fi
    if grep -q 'p = {x = 1, y = 2, tag = 3' gdb2.txt; then ok "gdb: struct value"; else bad "gdb: struct value"; fi
else
    echo "SKIP: gdb checks (gdb not installed)"
fi

echo "g3 tests: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
