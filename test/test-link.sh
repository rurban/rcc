#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Link tests for the rcc driver: shared libraries (.so/.dll/.dylib),
# static archives (.a), a Windows import library (.lib), plus a large
# real-world case (the sqlite3.c amalgamation).  tinycc-tests-style:
# self-contained, prints PASS/FAIL per case, exits non-zero if any case
# fails.
#
# Each case builds artifacts with rcc and runs the resulting program,
# checking its output/exit status -- so it validates the whole pipeline
# (shared-object writer, dylib export trie + dyld binds, archive member
# extraction) end to end, whichever internal path rcc takes.
#
# Usage: ./test/link-test.sh [rcc-binary]      (default: ./rcc)

cd "$(dirname "$0")/.." || exit 1
RCC="${1:-./rcc}"
AR="${AR:-ar}"

if [ ! -x "$RCC" ]; then
    echo "ERROR: rcc not found at '$RCC'. Build it first (make)." >&2
    exit 1
fi
# Absolutize rcc: the cases below cd into a scratch dir.
RCC="$(cd "$(dirname "$RCC")" && pwd)/$(basename "$RCC")"
ROOT="$(pwd)"

case "$(uname -s)" in
    Darwin)               SOEXT=dylib ;;
    MINGW*|MSYS*|CYGWIN*) SOEXT=dll ;;
    *)                    SOEXT=so ;;
esac

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT INT TERM
PASS=0
FAIL=0

pass() { PASS=$((PASS + 1)); printf '  %-44s OK\n'   "$1"; }
fail() { FAIL=$((FAIL + 1)); printf '  %-44s FAIL (%s)\n' "$1" "$2"; }
# Run a freshly linked program with the scratch dir on the library path so a
# .so/.dll built there is found regardless of its recorded soname.
runlib() { DYLD_LIBRARY_PATH="$TMP" LD_LIBRARY_PATH="$TMP" "$@"; }

# sqlite amalgamation (cached in bench/), same source the benchmark uses.
SQLITE_URL="https://sqlite.org/2026/sqlite-amalgamation-3530200.zip"
download_sqlite() {
    [ -f "$ROOT/bench/sqlite3.c" ] && return 0
    if command -v curl >/dev/null 2>&1; then
        curl -sSL "$SQLITE_URL" -o "$TMP/sqlite.zip" || return 1
    elif command -v wget >/dev/null 2>&1; then
        wget -q "$SQLITE_URL" -O "$TMP/sqlite.zip" || return 1
    else
        return 1
    fi
    unzip -o -j "$TMP/sqlite.zip" "sqlite-amalgamation-*/sqlite3.c" \
        "sqlite-amalgamation-*/sqlite3.h" -d "$ROOT/bench/" >/dev/null 2>&1
}

echo "Link tests (rcc=$RCC, ext=.$SOEXT)"

# ---------------------------------------------------------------------------
# 1. Small shared library: build a .so/.dylib/.dll and consume it.
# ---------------------------------------------------------------------------
cat > "$TMP/greet.c" <<'EOF'
int   gcount = 40;                       /* exported data */
int   add2(int a, int b) { return a + b; }
const char *tag(void)    { return "greetlib"; }
EOF
cat > "$TMP/gmain.c" <<'EOF'
extern int   add2(int, int);
extern int   gcount;
extern const char *tag(void);
extern int   strcmp(const char *, const char *);
int main(void) {
    if (add2(gcount, 2) != 42)          return 1;   /* imported func + data */
    if (strcmp(tag(), "greetlib") != 0) return 2;   /* imported string ptr  */
    return 0;
}
EOF
if "$RCC" -shared -fPIC "$TMP/greet.c" -o "$TMP/libgreet.$SOEXT" 2>"$TMP/e1" \
    && "$RCC" "$TMP/gmain.c" "$TMP/libgreet.$SOEXT" -o "$TMP/gprog" 2>>"$TMP/e1" \
    && runlib "$TMP/gprog"; then
    pass "shared library build + consume (.$SOEXT)"
else
    fail "shared library build + consume (.$SOEXT)" "$(tr '\n' ' ' < "$TMP/e1")"
fi

# ---------------------------------------------------------------------------
# 2. Static archive: bundle two rcc objects with ar and link against it.
#    Self-contained (no libc) so rcc resolves the members itself.
# ---------------------------------------------------------------------------
cat > "$TMP/a1.c" <<'EOF'
int square(int x) { return x * x; }
EOF
cat > "$TMP/a2.c" <<'EOF'
int cube(int x) { return x * x * x; }
EOF
cat > "$TMP/amain.c" <<'EOF'
extern int square(int), cube(int);
int main(void) { return (square(5) + cube(3) == 52) ? 0 : 1; } /* 25+27 */
EOF
if "$RCC" -c "$TMP/a1.c" -o "$TMP/a1.o" 2>"$TMP/e2" \
    && "$RCC" -c "$TMP/a2.c" -o "$TMP/a2.o" 2>>"$TMP/e2" \
    && "$AR" rcs "$TMP/libmath.a" "$TMP/a1.o" "$TMP/a2.o" 2>>"$TMP/e2" \
    && "$RCC" "$TMP/amain.c" "$TMP/libmath.a" -o "$TMP/aprog" 2>>"$TMP/e2" \
    && "$TMP/aprog"; then
    pass "static archive build + link (.a)"
else
    fail "static archive build + link (.a)" "$(tr '\n' ' ' < "$TMP/e2")"
fi

# ---------------------------------------------------------------------------
# 3 & 4. Large real-world case: sqlite3.c as a shared library and as a
#    static archive.  The driver opens an in-memory DB, round-trips a value,
#    and prints the library version.
# ---------------------------------------------------------------------------
if download_sqlite; then
    cat > "$TMP/sqdrv.c" <<'EOF'
#include <stdio.h>
#include "sqlite3.h"
int main(void) {
    sqlite3 *db; sqlite3_stmt *st; int x = -1;
    if (sqlite3_open(":memory:", &db))                                  return 1;
    if (sqlite3_exec(db, "CREATE TABLE t(x);INSERT INTO t VALUES(42);",
                     0, 0, 0))                                          return 2;
    if (sqlite3_prepare_v2(db, "SELECT x FROM t", -1, &st, 0))          return 3;
    if (sqlite3_step(st) != SQLITE_ROW)                                 return 4;
    x = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    sqlite3_close(db);
    printf("sqlite %s x=%d\n", sqlite3_libversion(), x);
    return x == 42 ? 0 : 5;
}
EOF
    # 3. sqlite as a shared library (linked natively by rcc's dylib writer).
    if "$RCC" -shared -fPIC -I"$ROOT/bench" "$ROOT/bench/sqlite3.c" \
            -o "$TMP/libsqlite3.$SOEXT" 2>"$TMP/e3" \
        && "$RCC" -I"$ROOT/bench" "$TMP/sqdrv.c" "$TMP/libsqlite3.$SOEXT" \
            -o "$TMP/sqsh" 2>>"$TMP/e3" \
        && [ "$(runlib "$TMP/sqsh" 2>>"$TMP/e3" | sed -n 's/.*\(x=[0-9]*\)/\1/p')" = "x=42" ]; then
        pass "sqlite3 shared library (.$SOEXT)"
    else
        fail "sqlite3 shared library (.$SOEXT)" "$(tr '\n' ' ' < "$TMP/e3")"
    fi

    # 4. sqlite as a static archive.
    if "$RCC" -c -I"$ROOT/bench" "$ROOT/bench/sqlite3.c" -o "$TMP/sqlite3.o" 2>"$TMP/e4" \
        && "$AR" rcs "$TMP/libsqlite3.a" "$TMP/sqlite3.o" 2>>"$TMP/e4" \
        && "$RCC" -I"$ROOT/bench" "$TMP/sqdrv.c" "$TMP/libsqlite3.a" \
            -o "$TMP/sqar" 2>>"$TMP/e4" \
        && [ "$("$TMP/sqar" 2>>"$TMP/e4" | sed -n 's/.*\(x=[0-9]*\)/\1/p')" = "x=42" ]; then
        pass "sqlite3 static archive (.a)"
    else
        fail "sqlite3 static archive (.a)" "$(tr '\n' ' ' < "$TMP/e4")"
    fi
else
    printf '  %-44s SKIP (no sqlite3.c, no curl/wget)\n' "sqlite3 large case"
fi

# ---------------------------------------------------------------------------
# 5. Windows import library (.lib): build a DLL together with a GNU-style
#    import library via -Wl,--out-implib, then link the consumer against
#    the .lib -- not the .dll directly, the normal way Windows code
#    consumes a shared library.  This exercises a code path case 1 never
#    touches: the driver's own ".lib" file-classification (main.c treats
#    it as a link input only when built for Windows) and the linker's
#    import-library symbol resolution, as opposed to case 1's direct
#    linking against the .dll's export table.
# ---------------------------------------------------------------------------
if [ "$SOEXT" = dll ]; then
    if "$RCC" -shared -fPIC "$TMP/greet.c" -o "$TMP/libgreet2.dll" \
            -Wl,--out-implib,"$TMP/libgreet2.lib" 2>"$TMP/e5" \
        && [ -f "$TMP/libgreet2.lib" ] \
        && "$RCC" "$TMP/gmain.c" "$TMP/libgreet2.lib" -o "$TMP/gprog2" 2>>"$TMP/e5" \
        && runlib "$TMP/gprog2"; then
        pass "shared library + import lib link (.lib)"
    else
        fail "shared library + import lib link (.lib)" "$(tr '\n' ' ' < "$TMP/e5")"
    fi
else
    printf '  %-44s SKIP (not a Windows target)\n' "shared library + import lib link (.lib)"
fi

# ---------------------------------------------------------------------------
# 6. -rdynamic: a dlopen()'d plugin calling back into a symbol *defined in
#    the main executable* -- the shape bash's loadable builtins use
#    (enable -f ./strmatch.so strmatch, which calls back into bash's own
#    lib/glob/strmatch.c strmatch()). Without -rdynamic, a plain
#    executable's .dynsym holds only the imports needed to bind against
#    shared libraries -- none of its own definitions -- so dlopen()'s
#    lazy binding of the plugin's undefined reference fails at load time.
#    ELF/Linux-specific (real gcc's -rdynamic has no equivalent effect on
#    Windows/macOS import/export models).
# ---------------------------------------------------------------------------
if [ "$SOEXT" = so ]; then
    cat > "$TMP/rdmain.c" <<'EOF'
#include <dlfcn.h>
int exported_value(void) { return 42; }
int main(void) {
    void *h = dlopen("./rdplugin.so", RTLD_NOW);
    if (!h) return 1;
    int (*fn)(void) = (int (*)(void))dlsym(h, "call_exported");
    if (!fn) return 2;
    return fn() == 43 ? 0 : 3;
}
EOF
    cat > "$TMP/rdplugin.c" <<'EOF'
extern int exported_value(void);
int call_exported(void) { return exported_value() + 1; }
EOF
    if "$RCC" -rdynamic "$TMP/rdmain.c" -ldl -o "$TMP/rdmain" 2>"$TMP/e6" \
        && "$RCC" -shared -fPIC "$TMP/rdplugin.c" -o "$TMP/rdplugin.$SOEXT" 2>>"$TMP/e6" \
        && ( cd "$TMP" && ./rdmain ); then
        pass "-rdynamic dlopen callback (.$SOEXT)"
    else
        fail "-rdynamic dlopen callback (.$SOEXT)" "$(tr '\n' ' ' < "$TMP/e6")"
    fi
else
    printf '  %-44s SKIP (Linux/ELF-only)\n' "-rdynamic dlopen callback"
fi

echo ""
echo "Link tests: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
