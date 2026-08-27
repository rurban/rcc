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

# SOEXT is driven by the *target* rcc binary, not the host: a mingw-cross
# rcc.exe (invoked directly here -- binfmt_misc/wine runs a .exe
# transparently on this Linux host, same as run_tests.exe does) still
# targets Windows even though `uname -s` reports Linux.
case "$RCC" in
    *.exe) SOEXT=dll ;;
    *)
        case "$(uname -s)" in
            Darwin)               SOEXT=dylib ;;
            MINGW*|MSYS*|CYGWIN*) SOEXT=dll ;;
            *)                    SOEXT=so ;;
        esac
        ;;
esac

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT INT TERM
PASS=0
FAIL=0

pass() { PASS=$((PASS + 1)); printf '  %-44s OK\n'   "$1"; }
fail() { FAIL=$((FAIL + 1)); printf '  %-44s FAIL (%s)\n' "$1" "$2"; }
# Run a freshly linked program with the scratch dir on the library path so a
# .so/.dll built there is found regardless of its recorded soname.
runlib() {
    prog="$1"
    shift
    # A cross-built rcc.exe or a Windows-hosted rcc always produces
    # "$prog.exe" for a plain executable link (both the native PE
    # linker and the external gcc.exe fallback append it); resolve
    # whichever name a given build step actually produced.
    [ ! -f "$prog" ] && [ -f "$prog.exe" ] && prog="$prog.exe"
    DYLD_LIBRARY_PATH="$TMP" LD_LIBRARY_PATH="$TMP" "$prog" "$@"
}

# Resolve a built program's actual on-disk path (see runlib()'s comment):
# echoes "$1" unchanged unless only "$1.exe" exists.
winprog() {
    if [ ! -f "$1" ] && [ -f "$1.exe" ]; then
        printf '%s' "$1.exe"
    else
        printf '%s' "$1"
    fi
}

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
    && "$(winprog "$TMP/aprog")"; then
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
        && [ "$("$(winprog "$TMP/sqar")" 2>>"$TMP/e4" | sed -n 's/.*\(x=[0-9]*\)/\1/p')" = "x=42" ]; then
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
# 6. Windows import library, GNU-idiomatic extension (.dll.a): same
#    shape as case 5 but names the import library the way mingw/autotools
#    build systems conventionally do (libfoo.dll.a, vs. MSVC-style
#    libfoo.lib) -- confirms the driver's link-input classification (a
#    positional file's *final* extension, ".a", already covers this; see
#    main.c) and the linker's import-library resolution are extension-
#    agnostic, not hardcoded to ".lib".
# ---------------------------------------------------------------------------
if [ "$SOEXT" = dll ]; then
    if "$RCC" -shared -fPIC "$TMP/greet.c" -o "$TMP/libgreet3.dll" \
            -Wl,--out-implib,"$TMP/libgreet3.dll.a" 2>"$TMP/e5b" \
        && [ -f "$TMP/libgreet3.dll.a" ] \
        && "$RCC" "$TMP/gmain.c" "$TMP/libgreet3.dll.a" -o "$TMP/gprog3" 2>>"$TMP/e5b" \
        && runlib "$TMP/gprog3"; then
        pass "shared library + import lib link (.dll.a)"
    else
        fail "shared library + import lib link (.dll.a)" "$(tr '\n' ' ' < "$TMP/e5b")"
    fi
else
    printf '  %-44s SKIP (not a Windows target)\n' "shared library + import lib link (.dll.a)"
fi

# ---------------------------------------------------------------------------
# 7. Import library, data-only export: a DLL that exports a plain data
#    symbol and no function at all, consumed as an ordinary `extern`
#    (no __declspec(dllimport), the common case since rcc implements no
#    dllimport semantics -- see test_declspec_native_reject.c). The
#    generated import-library member for a *function* export defines a
#    callable `jmp`-through-IAT thunk under the plain symbol name; doing
#    that for a *data* export too would satisfy the reference with raw
#    machine-code bytes read as an int instead of leaving it genuinely
#    undefined so GNU ld's runtime-pseudo-relocation auto-import pass
#    (which requires the plain name to stay unresolved, with only
#    __imp_<name> defined) ever fires -- exactly the bug this case
#    reproduces (found via case 5 sharing greet.c's `gcount` global,
#    which happened to mask it because `add2`/`tag` also resolved
#    correctly; isolated here with *no* function export at all).
# ---------------------------------------------------------------------------
if [ "$SOEXT" = dll ]; then
    cat > "$TMP/onlydata.c" <<'EOF'
int answer = 42;
EOF
    cat > "$TMP/dmain.c" <<'EOF'
extern int answer;
int main(void) { return answer == 42 ? 0 : 1; }
EOF
    if "$RCC" -shared -fPIC "$TMP/onlydata.c" -o "$TMP/libonlydata.dll" \
            -Wl,--out-implib,"$TMP/libonlydata.lib" 2>"$TMP/e5c" \
        && [ -f "$TMP/libonlydata.lib" ] \
        && "$RCC" "$TMP/dmain.c" "$TMP/libonlydata.lib" -o "$TMP/dprog" 2>>"$TMP/e5c" \
        && runlib "$TMP/dprog"; then
        pass "import lib data-only export (.lib)"
    else
        fail "import lib data-only export (.lib)" "$(tr '\n' ' ' < "$TMP/e5c")"
    fi
else
    printf '  %-44s SKIP (not a Windows target)\n' "import lib data-only export (.lib)"
fi

# ---------------------------------------------------------------------------
# 8. -rdynamic: a dlopen()'d plugin calling back into a symbol *defined in
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

# ---------------------------------------------------------------------------
# 9. Plain (non-extern) GNU89 `inline` + `__attribute__((gnu_inline))`:
#    a common glibc/gperf-generated-code portability idiom (`#ifdef
#    __GNUC_STDC_INLINE__ __attribute__((gnu_inline)) #endif` right after
#    an `inline` function definition, forcing GNU89 semantics regardless
#    of the C99/C11 __GNUC_STDC_INLINE__ predefine) must still emit an
#    ordinary global-linkage definition -- only `extern inline` +
#    gnu_inline (a declaration-only stub) suppresses one. rcc's fn_exported
#    computation implemented plain C99 inline linkage unconditionally,
#    ignoring gnu_inline except in the already-handled extern+gnu_inline
#    case, so a plain gnu_inline function's body compiled fine per-TU but
#    was emitted as a local (non-.globl) symbol -- invisible to any other
#    translation unit, exactly the shape of hoedown's gperf-generated
#    html_blocks.c: html_blocks.o built without error, but the final
#    executable link failed with an undefined reference.
# ---------------------------------------------------------------------------
cat > "$TMP/gi_def.c" <<'EOF'
__inline
#ifdef __GNUC_STDC_INLINE__
__attribute__((__gnu_inline__))
#endif
int gnu_inline_answer(void) { return 42; }
EOF
cat > "$TMP/gi_main.c" <<'EOF'
int gnu_inline_answer(void);
int main(void) { return gnu_inline_answer() == 42 ? 0 : 1; }
EOF
if "$RCC" -c "$TMP/gi_def.c" -o "$TMP/gi_def.o" 2>"$TMP/e7" \
    && "$RCC" -c "$TMP/gi_main.c" -o "$TMP/gi_main.o" 2>>"$TMP/e7" \
    && "$RCC" "$TMP/gi_def.o" "$TMP/gi_main.o" -o "$TMP/giprog" 2>>"$TMP/e7" \
    && "$(winprog "$TMP/giprog")"; then
    pass "plain gnu_inline function export (2-TU link)"
else
    fail "plain gnu_inline function export (2-TU link)" "$(tr '\n' ' ' < "$TMP/e7")"
fi

# ---------------------------------------------------------------------------
# 10. Wide string literal alignment survives a 2-TU link. Each object's own
#    .rodata can be internally self-padded so its wchar_t data starts on a
#    4-byte boundary (Linux wchar_t is UTF-32), but elf_write.c used to
#    hardcode .rodata's ELF sh_addralign to 1 in every .o it wrote -- so
#    the linker (rcc's own, or a real system ld) was free to concatenate
#    this object's .rodata immediately after another object's, at whatever
#    odd byte offset that left, silently destroying the padding. glibc's
#    vectorized wcslen()/wmemcmp() then read the misaligned literal wrong.
#    a.o's .rodata deliberately ends on a non-multiple-of-4 byte count (a
#    1-byte narrow string) so b.o's wide literal is forced off a 4-byte
#    boundary once merged unless the fix holds.
# ---------------------------------------------------------------------------
cat > "$TMP/wa.c" <<'EOF'
const char pad1[] = "1234567";  /* 8 bytes incl NUL */
const char pad2 = 'Q';          /* 1 byte: forces b.o's rodata off a 4-byte boundary */
EOF
# A hand-written wcslen() prototype (not <wchar.h>) deliberately: mingw's
# real <wchar.h> pulls in a large family of __mingw_ovr (static inline,
# __attribute__((unused))) wide-stdio wrapper helpers this TU never
# calls; eliminate_unused_static_inline() (opt.c) is disabled outright
# on the mingw target for an unrelated, documented reason (corrupts an
# emulated-TLS layout elsewhere -- see that function's own comment), so
# none of those unused helpers get dropped there, and several reference
# UCRT-internal symbols this test's own link command doesn't pull in.
# The real, vectorized libc wcslen() is what this test needs to exercise
# -- its declaration doesn't have to come from <wchar.h>.
cat > "$TMP/wb.c" <<'EOF'
#include <stddef.h>
extern size_t wcslen(const wchar_t *s);
int main(void) {
    const wchar_t *p = L"symlinkname2";
    return wcslen(p) == 12 ? 0 : 1;
}
EOF
if "$RCC" -c "$TMP/wa.c" -o "$TMP/wa.o" 2>"$TMP/e8" \
    && "$RCC" -c "$TMP/wb.c" -o "$TMP/wb.o" 2>>"$TMP/e8" \
    && "$RCC" "$TMP/wa.o" "$TMP/wb.o" -o "$TMP/wprog" 2>>"$TMP/e8" \
    && "$(winprog "$TMP/wprog")"; then
    pass "wide string alignment survives 2-TU link"
else
    fail "wide string alignment survives 2-TU link" "$(tr '\n' ' ' < "$TMP/e8")"
fi

# ---------------------------------------------------------------------------
# 11. `#pragma once` must be scoped per translation unit, not per process.
#    preprocess() (called once per input file by main()'s multi-file
#    compile loop) already reset macro state via clear_macros(), but
#    never reset the separate once_files list -- so a single invocation
#    compiling more than one .c file directly to an executable (no -c,
#    e.g. `rcc a.c b.c -o prog`, exactly how many real-world Makefiles
#    build a multi-file program in one command) silently dropped a
#    #pragma-once'd header's entire contents (typedefs, prototypes,
#    macros) for every file after the first one that included it --
#    found via elk's own `main.c`/`elk.c` build (elk.h uses `#pragma
#    once`), which mis-parsed with "expected ';' or ','" on the very
#    first use of a typedef the header defines.
# ---------------------------------------------------------------------------
cat > "$TMP/po_hdr.h" <<'EOF'
#pragma once
typedef unsigned long po_typedef_t;
int po_func(int x);
#define PO_MACRO 42
EOF
cat > "$TMP/po_a.c" <<'EOF'
#include "po_hdr.h"
int po_func(int x) { return x + PO_MACRO; }
EOF
cat > "$TMP/po_b.c" <<'EOF'
#include "po_hdr.h"
int main(void) {
    po_typedef_t v = 5;
    return (int)v == 5 ? po_func(0) - PO_MACRO : 1;
}
EOF
if ( cd "$TMP" && "$RCC" -I"$TMP" -c "$TMP/po_a.c" "$TMP/po_b.c" ) 2>"$TMP/e9" \
    && "$RCC" "$TMP/po_a.o" "$TMP/po_b.o" -o "$TMP/poprog" 2>>"$TMP/e9" \
    && "$(winprog "$TMP/poprog")"; then
    pass "#pragma once scoped per-TU in a multi-file single invocation"
else
    fail "#pragma once scoped per-TU in a multi-file single invocation" "$(tr '\n' ' ' < "$TMP/e9")"
fi

# ---------------------------------------------------------------------------
# 12. Direct single-invocation multi-.c-to-executable link (`rcc a.c b.c
#    -o prog`, no -c) -- distinct from every case above, which either
#    links pre-built .o/.a inputs or (case 11) still separately -c's each
#    file first. This is the exact shape of a plain `$(CC) a.c b.c -o
#    prog` Makefile rule (e.g. elk's native `elk:` target): codegen
#    produces each file's object data in-process and hands it straight
#    to rcc_link() without ever writing an intermediate .o to disk.
#    rcc's native PE writer (link_pe.c) merges multiple COFF objects'
#    same-named sections (.text/.data/...) by concatenation, and did
#    correctly rebase each object's own RELOCATION offsets by where its
#    bytes landed in the merged section -- but never applied that same
#    rebase to its SYMBOL VALUES, so any symbol defined in the second (or
#    later) linked object resolved to the wrong address: main.o's call to
#    an external bfn() defined in a separately-compiled bfn.o landed on
#    whatever code happened to sit at bfn's offset WITHIN bfn.o's own
#    object (here, offset 0 -- main's own address), so main() silently
#    called itself and stack-overflowed instead of calling bfn(). ELF's
#    loader (link_elf.c) already rebases symbol values correctly; PE's
#    did not. b.c's function is deliberately defined AFTER a.c's own use
#    of it, and BOTH files carry an unrelated top-level global so the
#    two objects' merged .data offsets also differ from zero, so a
#    latent bug here cannot hide behind an all-zero coincidence.
# ---------------------------------------------------------------------------
cat > "$TMP/mca.c" <<'EOF'
int a_pad = 7;
extern int mc_bfn(void);
int main(void) { return mc_bfn() == 42 ? 0 : 1; }
EOF
cat > "$TMP/mcb.c" <<'EOF'
int b_pad = 9;
int mc_bfn(void) { return 42; }
EOF
if "$RCC" "$TMP/mca.c" "$TMP/mcb.c" -o "$TMP/mcprog" 2>"$TMP/e10" \
    && "$(winprog "$TMP/mcprog")"; then
    pass "direct multi-.c single-invocation executable link"
else
    fail "direct multi-.c single-invocation executable link" "$(tr '\n' ' ' < "$TMP/e10")"
fi

# ---------------------------------------------------------------------------
# 13. A versioned shared-library SONAME with a trailing SemVer-style
#    "-<prerelease>" tag (e.g. "libfoo.so.2.0.0-dev") passed as a
#    positional link input -- main.c's is_shared_lib_path() only
#    recognized a bare ".so"/".dylib" suffix, or one followed by
#    ".<digits>" version components, so anything after the version
#    digits (even just a trailing "-dev") fell through and rcc tried to
#    compile the binary .so as C source ("invalid token \x7fELF").
#    Found via nng's own CMake build (NNG_ABI_VERSION embeds a "-dev"/
#    "-rc1"-style NNG_PRERELEASE suffix straight into the SONAME:
#    libnng.so -> libnng.so.1 -> libnng.so.2.0.0-dev), which broke
#    every link step consuming the library positionally.
# ---------------------------------------------------------------------------
if [ "$SOEXT" != dll ]; then
    if "$RCC" -shared -fPIC "$TMP/greet.c" -o "$TMP/libgreet4.$SOEXT.2.0.0-dev" 2>"$TMP/e11" \
        && "$RCC" "$TMP/gmain.c" "$TMP/libgreet4.$SOEXT.2.0.0-dev" -o "$TMP/gprog4" 2>>"$TMP/e11" \
        && runlib "$TMP/gprog4"; then
        pass "versioned SONAME with SemVer prerelease suffix"
    else
        fail "versioned SONAME with SemVer prerelease suffix" "$(tr '\n' ' ' < "$TMP/e11")"
    fi
else
    printf '  %-44s SKIP (not applicable to Windows SONAMEs)\n' "versioned SONAME with SemVer prerelease suffix"
fi

# ---------------------------------------------------------------------------
# 14. Plain C99 `inline` (no `extern`, no `gnu_inline`) function whose
#    address is taken in more than one TU. Under standard C99 inline
#    linkage, an `inline`-only definition provides no forcing external
#    definition, so `&fn` in any such TU still needs a real out-of-line
#    body somewhere (taking an address can never be satisfied by pure
#    inlining) -- resolved at link time against whichever TU's identical
#    body the linker keeps, so every TU's `&fn` compares equal. rcc's
#    codegen emitted each non-exported TU's own copy as SB_LOCAL (a
#    private, non-collapsible per-TU symbol), so `&fn` differed across
#    TUs -- found via mpack's own test suite, which explicitly asserts
#    `&mpack_tag_nil` compares equal between two TUs that each only see
#    the plain-`inline` definition (SIGABRT on the assertion failure).
#    Fixed by emitting these as SB_WEAK instead (codegen.c) plus teaching
#    rcc's own native linker's duplicate-symbol check strong-overrides-
#    weak precedence (link.c), matching real GCC/Clang's final linked
#    behavior without the larger surgery of never emitting a body at all
#    (which is what GCC/Clang's own object files do).
# ---------------------------------------------------------------------------
cat > "$TMP/ilp.c" <<'EOF'
extern inline int il_answer(void) { return 42; }
EOF
cat > "$TMP/ila.c" <<'EOF'
inline int il_answer(void) { return 42; }
int (*il_pa)(void) = &il_answer;
EOF
cat > "$TMP/ilb.c" <<'EOF'
inline int il_answer(void) { return 42; }
extern int (*il_pa)(void);
int (*il_pb)(void) = &il_answer;
int main(void) {
    return (il_pa == il_pb && il_pb == &il_answer && il_answer() == 42) ? 0 : 1;
}
EOF
if "$RCC" "$TMP/ilp.c" "$TMP/ila.c" "$TMP/ilb.c" -o "$TMP/ilprog" 2>"$TMP/e12" \
    && "$(winprog "$TMP/ilprog")"; then
    pass "plain C99 inline function address uniqueness (3-TU link)"
else
    fail "plain C99 inline function address uniqueness (3-TU link)" "$(tr '\n' ' ' < "$TMP/e12")"
fi

# ---------------------------------------------------------------------------
# 15. A `-g`-built shared library, dlopen()'d. link_elf.c's .rela.dyn
#    emission (R_X86_64_RELATIVE/R_X86_64_64 entries) iterated every
#    section including non-allocated debug sections (.debug_info/
#    .debug_line/.debug_aranges), which carry their own DWARF-internal
#    relocations describing offsets *within the debug data itself* --
#    unrelated to the runtime image, and never meant to land in
#    .rela.dyn. Those bogus entries recorded tiny, low offsets (e.g. 0x10)
#    against sec->addr=0 (matching .text's own base for -shared), which
#    ld.so then wrote an absolute pointer *into* .text -- a read-only,
#    executable segment -- corrupting the running process (or crashing
#    outright, deep inside glibc's own elf_machine_rela) the moment
#    dlopen() processed them. A locally-defined function pointer forces
#    the R_*_RELATIVE entry this needs; `-g` forces the debug sections
#    that used to corrupt it. Found via chibi-scheme's own `-g3` shared-
#    library modules (every lib/*.so): dlopen() crashed unconditionally.
#    link_macho.c had the analogous bug in a different shape: its object
#    loader folded __DWARF debug section bytes straight into __TEXT,
#    __const (rcc's ".rdata"), bloating/corrupting real rodata content
#    instead of crashing outright (Mach-O's own rebase-opcode builder
#    happens to filter by segment address range, which incidentally
#    excluded the bogus entries from ITS OWN output) -- confirmed via
#    a native `rcc-darwin` build (see darwin-test.sh), inspecting the
#    raw Mach-O bytes directly (no dyld available on this Linux host):
#    without the fix, __TEXT,__const carried 224 bytes of merged DWARF
#    garbage for a program with no const data of its own at all.
#    dlopen() works identically on both ELF and Mach-O (POSIX dlfcn),
#    so this one case covers both platforms directly.
# ---------------------------------------------------------------------------
if [ "$SOEXT" = so ] || [ "$SOEXT" = dylib ]; then
    cat > "$TMP/dbglib.c" <<'EOF'
static int local_answer(void) { return 42; }
int (*answer_fn)(void) = local_answer; /* forces R_*_RELATIVE / a Mach-O rebase entry */
EOF
    dl_lib=""
    [ "$SOEXT" = so ] && dl_lib="-ldl"
    cat > "$TMP/dbgmain.c" <<EOF
#include <dlfcn.h>
int main(void) {
    void *h = dlopen("./libdbg.$SOEXT", RTLD_NOW);
    if (!h) return 1;
    int (**pfn)(void) = (int (**)(void))dlsym(h, "answer_fn");
    if (!pfn || !*pfn) return 2;
    return (*pfn)() == 42 ? 0 : 3;
}
EOF
    if "$RCC" -shared -fPIC -g -g3 -O3 "$TMP/dbglib.c" -o "$TMP/libdbg.$SOEXT" 2>"$TMP/e13" \
        && "$RCC" "$TMP/dbgmain.c" $dl_lib -o "$TMP/dbgmain" 2>>"$TMP/e13" \
        && ( cd "$TMP" && ./dbgmain ); then
        pass "shared library with debug info (.$SOEXT), dlopen'd"
    else
        fail "shared library with debug info (.$SOEXT), dlopen'd" "$(tr '\n' ' ' < "$TMP/e13")"
    fi
else
    printf '  %-44s SKIP (ELF/Mach-O only)\n' "shared library with debug info, dlopen'd"
fi

# ---------------------------------------------------------------------------
# 16. A `-g`-built DLL, directly linked and loaded -- Windows' equivalent
#    of case 15 (no dlfcn.h on Windows; a normal DLL import at process
#    load already applies the base relocation table, exercising the same
#    link_pe.c build_pe_reloc() path). Same root cause: debug sections
#    (.debug_line/.debug_info/.debug_abbrev/.debug_aranges, marked
#    IMAGE_SCN_MEM_DISCARDABLE by coff_write.c) carry their own internal
#    relocations that link_load_object() used to load back as ordinary
#    alloc=true sections, letting build_pe_reloc() (which already
#    correctly skips !sec->alloc) sweep them into spurious .reloc
#    entries anyway -- confirmed directly by inspecting the produced
#    DLL's raw .reloc bytes: without the fix, three extra DIR64 entries
#    appeared at the exact same debug-section-relative offsets
#    (0x10/0x28/0x37) as the ELF case. A locally-defined function
#    pointer forces the one real DIR64 entry this needs; `-g` forces
#    the debug sections that used to add bogus ones alongside it.
# ---------------------------------------------------------------------------
if [ "$SOEXT" = dll ]; then
    cat > "$TMP/dbglib.c" <<'EOF'
static int local_answer(void) { return 42; }
int (*answer_fn)(void) = local_answer;
EOF
    cat > "$TMP/dbgmain2.c" <<'EOF'
extern int (*answer_fn)(void);
int main(void) { return answer_fn() == 42 ? 0 : 1; }
EOF
    if "$RCC" -shared -fPIC -g -g3 -O3 "$TMP/dbglib.c" -o "$TMP/libdbg2.dll" 2>"$TMP/e14" \
        && "$RCC" "$TMP/dbgmain2.c" "$TMP/libdbg2.dll" -o "$TMP/dbgmain2" 2>>"$TMP/e14" \
        && runlib "$TMP/dbgmain2"; then
        pass "DLL with debug info (.dll), directly linked"
    else
        fail "DLL with debug info (.dll), directly linked" "$(tr '\n' ' ' < "$TMP/e14")"
    fi
else
    printf '  %-44s SKIP (Windows/PE only)\n' "DLL with debug info, directly linked"
fi

# ---------------------------------------------------------------------------
# 17. Repeated -l flags emit a single DT_NEEDED entry each. rcc's ELF
#    linker pre-seeds libc/libgcc_s/libm DT_NEEDED entries and then
#    appended one per -l<name> occurrence without dedup, so a link with
#    the same library twice (rcc's default libm plus an explicit -lm in
#    a Makefile/configure LIBS, or -lfoo -lfoo) produced duplicate
#    NEEDED entries -- real ld dedups to one. Harmless at runtime, but
#    configure probes that parse `objdump -p` NEEDED lines (gnutls'
#    M_LIBRARY_SONAME check) captured "libm.so.6\nlibm.so.6" and wrote
#    a broken config.h define. Count NEEDED libm.so.6 in the linked
#    binary's dynamic section: must be exactly one.
# ---------------------------------------------------------------------------
if [ "$SOEXT" = so ]; then
    cat > "$TMP/need.c" <<'EOF'
#include <math.h>
int main(void) { return trunc(1.5) == 1.0 ? 0 : 1; }
EOF
    if "$RCC" "$TMP/need.c" -lm -lm -o "$TMP/needprog" 2>"$TMP/e15"; then
        n=$(LC_ALL=C objdump -p "$TMP/needprog" 2>/dev/null | grep -c 'NEEDED.*libm\.so\.6')
        if [ "$n" = 1 ] && "$TMP/needprog"; then
            pass "repeated -lm emits one DT_NEEDED libm.so.6"
        else
            fail "repeated -lm emits one DT_NEEDED libm.so.6" "NEEDED libm count=$n"
        fi
    else
        fail "repeated -lm emits one DT_NEEDED libm.so.6" "$(tr '\n' ' ' < "$TMP/e15")"
    fi
else
    printf '  %-44s SKIP (ELF/Linux only)\n' "repeated -lm emits one DT_NEEDED libm.so.6"
fi

# ---------------------------------------------------------------------------
# 18. Weak-symbol interposition across a shared-library boundary. A
#    -shared link must route internal calls to defined global/weak
#    functions through the PLT (a direct call hard-wires the library's
#    own copy), and a non-shared executable must export its definitions of
#    symbols the linked libraries reference (GNU ld does both by default,
#    independent of -rdynamic). gnutls' GNUTLS_SKIP_GLOBAL_INIT relies on
#    exactly this: the test executable's strong _gnutls_global_init_skip
#    must override the library's weak one, or the lib's implicit-init
#    constructor runs anyway (failed tests/global-init-override). The
#    executable's strong gsk() (42) must win over the library's weak gsk()
#    (0) even WITHOUT -rdynamic.
# ---------------------------------------------------------------------------
if [ "$SOEXT" = so ]; then
    cat > "$TMP/wl.c" <<'EOF'
__attribute__((weak)) int gsk(void) { return 0; }
int call_gsk(void) { return gsk(); }
EOF
    cat > "$TMP/wm.c" <<'EOF'
int gsk(void) { return 42; }
int call_gsk(void);
int main(void) { return call_gsk() == 42 ? 0 : 1; }
EOF
    if "$RCC" -shared -fPIC "$TMP/wl.c" -o "$TMP/libweak.so" 2>"$TMP/e16" \
        && "$RCC" "$TMP/wm.c" "$TMP/libweak.so" -o "$TMP/weakprog" 2>>"$TMP/e16"; then
        if LD_LIBRARY_PATH="$TMP" "$TMP/weakprog"; then
            pass "weak symbol interposed from executable without -rdynamic"
        else
            fail "weak symbol interposed from executable without -rdynamic" "$(tr '\n' ' ' < "$TMP/e16")"
        fi
    else
        fail "weak symbol interposed from executable without -rdynamic" "$(tr '\n' ' ' < "$TMP/e16")"
    fi
else
    printf '  %-44s SKIP (ELF/Linux only)\n' "weak symbol interposed from executable without -rdynamic"
fi

echo ""
echo "Link tests: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
