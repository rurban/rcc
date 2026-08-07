# Third-Party Test Results & Triage TODO

Batch run: 2026-08-06 (199 of 221 targets)
Binary: rcc HEAD (third_party branch)

## Summary

| rc  | count | meaning                                                        |
| --- | ----- | -------------------------------------------------------------- |
| 0   | 54    | pass                                                           |
| 2   | 101   | build/compile failure                                          |
| 1   | 18    | runtime/test failure                                           |
| 124 | 12    | timeout (420 s)                                                |
| 127 | 10    | missing tool (muon, lzip, etc.)                                |
| 139 | 1     | SIGSEGV (box2d C++ binary, not rcc)                            |
| 8   | 2     | test failure (blake3)                                          |
| 6   | 1     | —                                                              |
| 1   | 18    | runtime/test failure (many are build-system: CC not respected) |
| 124 | 12    | timeout (420 s)                                                |
| 127 | 10    | missing tool (muon, lzip, etc.)                                |

## File Layout

**⚠ False positives**: Many projects (lua, mruby, many cmake projects)
hardcode `CC=gcc` in their Makefiles and ignore the environment. The test
harness sets `CC=rcc` but the build system overrides it. Verify by checking
`strings <binary> | grep GCC` — if it says GCC, rcc wasn't used.

**Genuine rcc bugs found so far**:

### Fixed (2026-08-07, continued)

- **Prototype/definition redeclaration not diagnosed** (parser.c) — rcc
  silently accepted incompatible re-declarations of the same function
  within one TU (`extern int f(int); int f(char x) {}`; `int rand(void);`
  redeclared `int rand(int n);`). Now:
  - Errors on a real type mismatch, at _any_ redeclaration point (not
    just the definition) — needed because glibc's own `<stdlib.h>` +
    `int rand(int n);` conflict is two plain declarations, not a
    declaration-then-definition.
  - Distinguishes explicit `(void)` (zero params) from K&R-style empty
    `()` (unspecified params, compatible with anything) via a new
    `Type::is_void_params` flag — both otherwise parse to the same
    empty `param_types` list, so redeclaring `int f(void)` as `int
f(int)` must error while `int f(); int f(int a) {...}` must not.
  - Warns (only with `-W`) on qualifier-only differences (`int
x[const 5]` vs `int (*const x)`) instead of erroring — these are
    the _same_ type per C, not a conflict.
  - Skips `extern __inline__` glibc fortify wrappers, old-style/K&R
    definitions, and struct/union-typed parameters (compared via
    `type_equal()`'s existing pointer-identity rule, see below).
    → unblocks: curl (configure's
    `CURL_CHECK_COMPILER_PROTOTYPE_MISMATCH` probe)
- **`type_equal()` struct/union pointer-identity check was too strict**
  (parser.c) — `declarator_params()` intentionally shallow-copies each
  parameter's `Type` node (`*pt = *pty`), so a struct/union parameter
  re-parsed from a second declaration was never pointer-identical to
  the first, even more so when nested inside a function-pointer
  parameter's own parameter list (copied _again_). Compare
  `members` (set once, when the tag is defined, and preserved
  unmodified by the shallow copy) instead of raw identity.
  → unblocks: mruby (`mrb_get_values_at`'s `mrb_value (*func)(...)`
  parameter false-flagged as a prototype conflict by the fix above)
- **glibc `_FORTIFY_SOURCE` `_chk`/`_chk_warn` builtins were bare
  object-macros** (preprocess.c) — `#define __read_chk read` etc.
  (added in `de077b2c` so real call sites from glibc's
  `__glibc_fortify` expansion resolve correctly) also fired on the
  _declaration_ of `__read_chk` itself in
  `<bits/unistd-decl.h>` (`extern ssize_t __read_chk(int, void*,
size_t, size_t);`, 4 params), textually producing `extern ssize_t
read(int, void*, size_t, size_t);` — 4 params, conflicting with
  `<unistd.h>`'s real 3-param `read` once the prototype check above
  started catching it. Converted all 32 of these (bare and
  `__builtin_`-prefixed, plus `_warn` siblings) to function-like
  macros that drop the trailing bufsize/buflen parameter: real calls
  still forward correctly, and the declaration's parameter list
  shrinks in step, keeping arity in sync automatically.
  → unblocks: curl, and any `_FORTIFY_SOURCE=2 -O1+` build
- **`include/assert.h` unconditionally included `<stdbool.h>`** — real
  glibc `<assert.h>` does not; this leaked `bool`/`true`/`false` as
  _macros_ into any TU that merely asserted, breaking `##`-pasted
  identifiers built from those names (e.g. flatcc's lexer generator:
  `#define lex_kw_match(kw) ... tok_kw_##kw`, called as
  `lex_kw_match(bool)` — `bool` got macro-expanded to `_Bool` _before_
  the paste per the `#`/`##` non-expansion rule already being honored
  correctly elsewhere, producing `tok_kw__Bool` instead of
  `tok_kw_bool`). Removed the include.
  → unblocks: flatcc (`keywords.h:19: error: undeclared variable
tok_kw__Bool`)
- **Compile-time constant folding of float subexpressions cast to an
  integer truncated too early** (parser.c, `eval_const_expr`) —
  `ND_FNUM` truncated each float literal to `long long` _at the leaf_,
  so `(size_t)(0.7f * 256.0f)` folded as `(long long)0.7f *
(long long)256.0f = 0 * 256 = 0` instead of `(long long)(0.7f *
256.0f) = 179`. Added a float-aware sibling evaluator
  (`eval_const_fexpr`, `long double` throughout) that
  `eval_const_expr` delegates to for any floating-typed subexpression,
  converting to an integer only once, at the actual cast boundary.
  → unblocks: flatcc — its hash-table load-factor threshold
  (`_flatcc_refmap_above_load_factor`) was permanently 0, so the
  resize loop's exit condition could never be met; `buckets *= 2`
  looped forever (eventually wrapping to 0 and spinning), hanging
  `monster_test` at 100% CPU the first time the table needed to grow.
- **`__builtin_ia32_pause`/`mfence`/`lfence`/`sfence` unimplemented**
  (preprocess.c) — real GCC/clang implement these as genuine compiler
  builtins requiring no header and no linkable symbol; code (curl's
  bundled curlx headers) calls them directly without going through
  `<emmintrin.h>`'s `_mm_pause()`. Added as inline-asm
  function-like macros (x86-only, gated on `!ARCH_ARM64`).
  → unblocks: curl (`undefined reference to '__builtin_ia32_pause'`
  at link time)

Regression tests: `test/test_err_proto_conflict.c`,
`test/test_proto_fnptr_struct_param.c`,
`test/test_proto_void_params.c` + `test/test_err_proto_void_params.c`,
`test/test_fortify_chk_arity.c`, `test/test_const_float_fold.c`,
`test/test_ia32_pause.c`. Full suite verified after each fix: TCC
118/118, Unit tests 163/163, Torture 3605/3609 (100% of non-skipped),
Dg-error 34/34, Link 4/4.

### Confirmed rcc bug, not yet fixed: test_mruby crash

`mrbtest` SIGSEGVs (stack overflow) during `mrb_mruby_objectspace_gem_init`,
deep inside infinite mutual recursion:
`convert_type` → `mrb_type_convert` → `mrb_funcall_with_block` →
`ensure_block` → `mrb_funcall_argv` → `convert_type` → ... (repeats
identically). `mrb_type_convert`'s failure path formats an error
message via `mrb_vformat`'s `%T`/`%Y` specifiers, which itself calls
`mrb_obj_as_string` → `mrb_type_convert` on the value being described;
if _that_ nested conversion also fails, it recurses into formatting
its own failure, without ever terminating.

**Confirmed real (not pre-existing/environment)**: the identical
mruby 4.0.0 checkout built with real gcc instead of rcc passes cleanly
(`Total: 1686, OK: 1677, KO: 0, Crash: 0, Skip: 9`). Something rcc
generates makes an object's method dispatch (`mrb_respond_to`)
incorrectly report "doesn't respond to :to_s" for a value that should.

Investigated but NOT reproduced in isolation:

- `mrb_value` is `{ union { double f; void *p; mrb_int i; ...} value;
enum mrb_vtype tt; }` (`boxing_no.h`) — a small struct passed BY
  VALUE pervasively, including through `mrb_raisef`'s `...` varargs
  and `mrb_vformat`'s `va_list` forwarding. Hypothesized a SysV ABI
  eightbyte-classification bug (union merging an SSE-class `double`
  member with INTEGER-class `void*`/`mrb_int` members at the same
  offset should classify INTEGER, per the AMD64 ABI merge rule "if one
  class is INTEGER, the result is INTEGER") that could put the struct
  in the wrong registers when it happens to hold a pointer. Built
  several targeted repros (direct call, single-level varargs, va_list
  forwarded through a non-variadic helper with mixed preceding
  int/string args) mirroring `mrb_raisef` → `error_va` →
  `mrb_vformat`'s exact shape — all matched gcc's output correctly.
  Root cause is likely deeper in the real call chain (GC interaction,
  register pressure at `-O3`, or mruby's own symbol/method hash-table
  lookup) and needs the actual mruby VM environment (not a minimal
  repro) plus a `-O0`/`-O1` vs `-O3` bisection to isolate further.

- `test/third_party/results.txt` — tab-separated: `rc\ttest_name\tduration`
- `test/third_party/logs/test_*.log` — per-project build + test output
- `test/third_party/test_<name>/` — failing project dirs (passing dirs auto-deleted)

Re-run a single test:
CC=$(pwd)/rcc bash test/linux*thirdparty.bash test*<name>

---

## rc=1 — Runtime Failures (builds OK, test fails)

| test             | symptom                                                                   |
| ---------------- | ------------------------------------------------------------------------- |
| test_lua         | db.lua:83 assertion: debug.getinfo(f).short_src                           |
| test_mruby       | mrbtest binary crashes — confirmed real rcc bug, see above                |
| test_curl        | **fixed** — was: configure "compiler does not halt on prototype mismatch" |
| test_c23doku     | C23 \_BitInt(N) not supported                                             |
| test_c3          | CMake: missing LLD_COFF                                                   |
| test_coremarkpro | benchmark runner can't find perf logs                                     |
| test_box3d       | C++ binary (g++ compiled, not rcc)                                        |
| test_glib        | —                                                                         |
| test_got         | configure: missing libbsd-overlay                                         |
| test_ksh93       | —                                                                         |
| test_libgmp      | configure: cannot determine 32-bit word directive                         |
| test_muon        | muon self-tests (some pass, some fail)                                    |
| test_neovim      | —                                                                         |
| test_nob         | git checkout only (build not reached?)                                    |
| test_rsync       | —                                                                         |
| test_samba       | —                                                                         |
| test_scrapscript | rcc compile fails (exit 1) during Python test harness                     |
| test_tcpdump     | —                                                                         |

---

## rc=2 — Build Failures (compile/link error)

Top root causes identified:

### Fixed (2026-08-06)

- `UINT_FAST64_MAX` / `INT_LEAST*_MAX` undeclared — **fixed** (stdint.h: added 24 missing limit macros)
  → unblocks: coreutils, diffutils, gpatch, gsed, gtar
- `__DATE__` / `__TIME__` undeclared — **fixed** (preprocess.c: added C89 predefined macros)
  → unblocks: mimalloc
- **memcmp**: with empty string
  → unblocks: lua
- **VA_OPT**: C23 variadic macro (bfs) — keyword registered but not expanded
  → unblocks: bfs
- **Prototype mismatch not diagnosed** — **fixed**, see "Fixed
  (2026-08-07, continued)" above for the full writeup
  → unblocks: curl

### Needs fixing

1. **Missing x86 intrinsics** (~10+ projects) — partially fixed
   (`__builtin_ia32_pause`/`mfence`/`lfence`/`sfence`, see above)
   - `_mm_shuffle_epi8` (SSSE3) → test_bearssl, test_blosc2, test_libflac
   - `__v8hi`, `__builtin_shufflevector` (GCC vector ext) → test_blake3, test_brotli, test_ffc, test_fftw, test_libwebp, ...
   - Root: rcc can't parse GCC's `<*mmintrin.h>` headers; these use `__v8hi` types and `__builtin_ia32_*` builtins

2. **C23 `_BitInt(N)`** — test_cproc, test_c23doku
   - `_BitInt(total * 3)` → "expected specific operator"

3. **lib/tempname.c pattern** (now partially fixed)
   - `SIZE_WIDTH` undeclared in test_diffutils (project-specific macro, not stdint)

4. **Object file passed as source** — test_heatshrink
   - `.os` file compiled as C source (build system issue, not rcc)

5. **Link failures (environment, not rcc)**: test_file, test_libgc, test_libjansson, ...
   - Missing system libs: libseccomp, libzstd, etc.

---

## rc=124 — Timeouts

| test              | notes                                                      |
| ----------------- | ---------------------------------------------------------- |
| test_bash         | rcc-compiled bash spins on alias4.sub — likely codegen bug |
| test_perl         | —                                                          |
| test_go           | —                                                          |
| test_nginx        | —                                                          |
| test_groff        | —                                                          |
| test_argtable3    | —                                                          |
| test_httpparser   | —                                                          |
| test_libarchive   | —                                                          |
| test_liblz4       | —                                                          |
| test_libpng       | —                                                          |
| test_libressl     | —                                                          |
| test_qbe_simplecc | —                                                          |

---

## Quick Wins (next to fix)

1. **test_mruby crash** — confirmed real rcc bug (stack overflow via
   infinite recursion in error formatting); needs gdb investigation
   with the actual mruby VM environment, see the detailed writeup
   above
2. **Missing x86 intrinsics**: `__v8hi`/`__builtin_shufflevector` GCC
   vector-extension surface — affects ~10 projects (blake3, brotli,
   bearssl, blosc2, fftw, libflac, libwebp, ...)
3. **C23 `_BitInt(N)`** — test_cproc, test_c23doku
