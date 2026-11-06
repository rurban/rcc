#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# -g3 (full DWARF) tests: DIEs, locations, lexical blocks, types, macros,
# debugger-visible behaviour, and the level split (-g/-g1 line table only,
# -g2 full, -g3 full + macros).
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

# Level split: -g/-g1 line table only, -g2 full without macros.
for lvl in -g -g1; do
    "$RCC" $lvl -c t.c -o t1.o 2>/dev/null
    if readelf --debug-dump=info t1.o 2>/dev/null | grep -q DW_TAG_subprogram; then
        bad "$lvl is line table only"
    else
        ok "$lvl is line table only"
    fi
done
"$RCC" -g2 -c t.c -o t2.o 2>/dev/null
if readelf --debug-dump=info t2.o 2>/dev/null | grep -q 'DW_AT_name .*: sq$'; then
    ok "-g2 emits subprograms"
else
    bad "-g2 emits subprograms"
fi
if readelf -S t2.o | grep -q debug_macinfo; then bad "-g2 has no macros"; else ok "-g2 has no macros"; fi

# Link + run with gcc and with rcc's own linker.
if gcc t.o -o t_gcc && ./t_gcc; then ok "gcc link, runs"; else bad "gcc link, runs"; fi
if "$RCC" -g3 t.c -o t_rcc && ./t_rcc; then ok "rcc link, runs"; else bad "rcc link, runs"; fi
if readelf --debug-dump=info t_rcc 2>/dev/null | grep -q DW_TAG_subprogram; then ok "rcc-linked exe keeps DIEs"; else bad "rcc-linked exe keeps DIEs"; fi

# --- call-frame information (.eh_frame): every -g level, rcc-linked or not ---
cat > cfi.c <<'EOF2'
#include <execinfo.h>
#include <pthread.h>
static int frames;
__attribute__((noinline)) int leaf(int x) {
    void *bt[32];
    int n = backtrace(bt, 32);
    if (n > frames) frames = n;
    return x + 1;
}
int mid(int a, int b, int c, int d, int e, int f, int g) {
    int arr[40];
    arr[a % 40] = g;
    return leaf(arr[a % 40] + b + c + d + e + f);
}
int top(int v) { return mid(v, 1, 2, 3, 4, 5, 6) + 1; }
static void *thr(void *p) { top(1); pthread_exit(p); return 0; }
int main(void) {
    top(3);
    pthread_t t;
    pthread_create(&t, 0, thr, 0);
    pthread_join(t, 0);
    return frames >= 6 ? 0 : 1; /* leaf mid top main + libc frames */
}
EOF2
"$RCC" -c cfi.c -o cfi0.o && if readelf -S cfi0.o | grep -q eh_frame; then bad "no .eh_frame without -g"; else ok "no .eh_frame without -g"; fi
for lvl in -g1 -g2 -g3; do
    "$RCC" $lvl -c cfi.c -o cfi.o 2>/dev/null
    if readelf -S cfi.o | grep -q '\.eh_frame' && readelf -wF cfi.o 2>/dev/null | grep -q 'rbp+16'; then ok "$lvl: .eh_frame with rbp frame rules"; else bad "$lvl: .eh_frame with rbp frame rules"; fi
done
"$RCC" -g -c cfi.c -o cfi.o
if gcc cfi.o -o cfi_gcc -lpthread && ./cfi_gcc; then ok "gcc link: backtrace/pthread_exit unwind through rcc frames"; else bad "gcc link: backtrace/pthread_exit unwind through rcc frames"; fi
if "$RCC" -g cfi.c -o cfi_rcc -lpthread && ./cfi_rcc; then ok "rcc link: backtrace/pthread_exit unwind through rcc frames"; else bad "rcc link: backtrace/pthread_exit unwind through rcc frames"; fi
if readelf -lW cfi_rcc | grep -q GNU_EH_FRAME; then ok "rcc link: PT_GNU_EH_FRAME"; else bad "rcc link: PT_GNU_EH_FRAME"; fi

# --- -g with the SSA IR backend (-O2): -g must not disable IR, and promoted
# locals get .debug_loc location lists (register + spill ranges).
cat > irloc.c <<'EOF2'
__attribute__((noinline)) int foo(int n) {
    int a = n + 2;
    int b = a * 3;
    return b - a;
}
int main(void) { return foo(5) == 15 ? 0 : 1; }
EOF2
"$RCC" -O2 -c irloc.c -o irloc_plain.o 2>/dev/null
"$RCC" -O2 -g -c irloc.c -o irloc_g.o 2>/dev/null
"$RCC" -O2 -g2 -c irloc.c -o irloc_g2.o 2>/dev/null
if cmp -s irloc_plain.o irloc_g.o; then bad "-O2 -g keeps the IR backend (identical code)"; else ok "-O2 -g keeps the IR backend (identical code)"; fi
if readelf -SW irloc_g2.o | grep -q "\.debug_loc"; then ok "-O2 -g2: .debug_loc location lists"; else bad "-O2 -g2: .debug_loc location lists"; fi
if readelf --debug-dump=info irloc_g2.o | grep -q "location list"; then ok "-O2 -g2: variables use location lists"; else bad "-O2 -g2: variables use location lists"; fi
if readelf -wF irloc_g2.o | grep -q "rbp+16"; then ok "-O2 -g2: .eh_frame for IR functions"; else bad "-O2 -g2: .eh_frame for IR functions"; fi
if readelf --debug-dump=decodedline irloc_g2.o | grep -q "irloc.c"; then ok "-O2 -g2: line table on IR code"; else bad "-O2 -g2: line table on IR code"; fi

if command -v gdb >/dev/null 2>&1; then
    gdb -batch -ex 'break sq' -ex run -ex 'info args' -ex 'up' -ex 'info locals' \
        -ex 'p gcount' -ex 'ptype struct Point' -ex 'macro expand SQUARE(3)' t_gcc > gdb1.txt 2>&1
    if grep -q '^v = 7' gdb1.txt; then ok "gdb: parameter value"; else bad "gdb: parameter value"; fi
    if grep -q 'int b5 : 5;' gdb1.txt; then ok "gdb: ptype bit-field"; else bad "gdb: ptype bit-field"; fi
    if grep -q "^[$]1 = 5" gdb1.txt; then ok "gdb: global value"; else bad "gdb: global value"; fi
    if grep -q '((3)\*(3))' gdb1.txt; then ok "gdb: macro expand"; else bad "gdb: macro expand"; fi
    "$RCC" -O2 -g3 irloc.c -o irloc_t && gdb -batch -ex 'break foo' -ex run -ex 'p n' \
        -ex 'next' -ex 'p a' -ex 'next' -ex 'p b' irloc_t > gdb4.txt 2>&1
    if grep -q "^[$]1 = 5" gdb4.txt && grep -q "^[$]2 = 7" gdb4.txt && grep -q "^[$]3 = 21" gdb4.txt; then
        ok "gdb: IR register location lists"
    else
        bad "gdb: IR register location lists"
    fi
    gdb -batch -ex 'break *sq' -ex run -ex bt -ex 'info args' t_gcc > gdb3.txt 2>&1
    if grep -q '^#1 .* main ' gdb3.txt; then ok "gdb: backtrace from the first instruction"; else bad "gdb: backtrace from the first instruction"; fi
    gdb -batch -ex 'break 15' -ex run -ex 'info locals' -ex 'p p' t_gcc > gdb2.txt 2>&1
    if grep -q 'local_i = ' gdb2.txt; then ok "gdb: block local"; else bad "gdb: block local"; fi
    if grep -q 'p = {x = 1, y = 2, tag = 3' gdb2.txt; then ok "gdb: struct value"; else bad "gdb: struct value"; fi
else
    echo "SKIP: gdb checks (gdb not installed)"
fi

echo "g3 tests: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
