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

### Fixed (2026-08-07, blosc2 session)

- **Inline-asm multi-output register clobber** (codegen.c) — a
  multi-output `asm()` using x86 fixed-register constraints (e.g.
  `"=a"`/`"=b"`/`"=c"`/`"=d"` for `cpuid`) could silently lose one
  output's value. codegen.c saves/restores an output's _address_
  register around the asm to protect it from being clobbered by the
  asm itself, but address-register allocation is independent per
  operand — operand j's address can land in the exact physical
  register operand i's own `"=b"` constraint targets. Popping
  operand j's saved address back into `%rbx` after the asm executed
  silently overwrote operand i's still-unread real result before the
  store-back loop could read it, with no diagnostic. Fixed by
  capturing every fixed-register output into a scratch vreg
  immediately after the asm executes, before any address-register
  restore runs.
  → found via blosc2's `blosc_get_cpu_features()` (four-output
  `cpuid` queries via `__builtin_cpu_supports()`)
- **Function-declarator parameter names leaked into file scope**
  (parser.c) — a function declarator's parameter names, from either
  a bare prototype or a full definition, were pushed onto the
  persistent file-scope `locals` list via `parse_params()`/`new_var()`
  but never cleared once the declarator finished (only the transient
  `current_fn_scope_locals` snapshot was reset). `find_var()` checks
  `locals` before falling back to globals/enum constants (required
  so a real local shadows a same-named global), so any later
  top-level identifier sharing a name with an earlier parameter —
  even a single letter like `f` — silently resolved to the stale,
  wrongly typed phantom parameter instead of its own declaration.
  Reproduced standalone with `int proto(const char *f); enum K { e,
f, g = f };` on the clean tree (pre-existing, not a regression from
  this session's other changes). Caught by torture test
  `c23-tag-enum-7` once a synthetic-prelude `__builtin_cpu_supports
(const char *f)` shadowed `enum K`'s own `f`.
- **`__builtin_cpu_supports("feature")`** (parser.c synthetic
  prelude, preprocess.c macro alias) — runtime CPU-dispatch compiler
  builtin used by several perf-sensitive libraries' SIMD-path
  selection. Implemented via a real `cpuid`-querying function
  injected into the x86-64 synthetic prelude (`static
__always_inline__` to avoid multi-TU link collisions).
- **SSE2 gaps**: `_mm_shufflelo_epi16`/`_mm_shufflehi_epi16`, the
  full `_mm_unpacklo`/`_mm_unpackhi_epi{8,16,32,64}` interleave
  family, `_mm_packs_epi16`/`_mm_packus_epi16`/`_mm_packs_epi32`
  saturating packs (emmintrin.h); `__m64` support via
  `_mm_storeh_pi`/`_mm_storel_pi` (xmmintrin.h).
- **SSSE3**: new `include/tmmintrin.h` (rcc had none) — everything
  expressible as plain lane-wise C arithmetic (abs/sign/hadd/hsub/
  alignr/maddubs/mulhrs) implemented via
  `__attribute__((vector_size))`; `_mm_shuffle_epi8` via a new
  `__builtin_ia32_pshufb128` encoder + dispatch (x86-only,
  x86_enc.c/h + codegen.c's `gen_vector_binary_builtin`).
  → unblocks: bearssl, blosc2, libflac (`_mm_shuffle_epi8`)
- **`stdint.h` `ptrdiff_t` typedef mismatch** — a pre-existing bug:
  rcc's own `stdint.h` typedef'd `ptrdiff_t` as `long`, conflicting
  with `<stddef.h>`'s correct `long long` once both got included in
  the same TU ("conflicting types for `ptrdiff_t`").

New regression tests: `test_asm_multi_output_clobber.c`,
`test_proto_param_scope_leak.c`, `test_sse2_unpack_pack.c`,
`test_m64_storehl_pi.c`, `test_ssse3_shuffle.c`,
`test_stdint_ptrdiff_conflict.c` (the last two guard their x86-only
portions with `#if !defined(__aarch64__) && !defined(_M_ARM64)`,
matching `test_ia32_pause.c`'s existing convention, so ARM64/macOS CI
keeps the portable coverage without failing on the x86-only bits).

**Verified**: c-blosc2 configures, builds (blosc + all
plugins/codecs), links, and its CTest suite runs cleanly (2100+
tests, no crashes, no hangs). One residual gap: a handful of
SSSE3-dispatch bitshuffle variants stay compiled out because CMake's
`check_c_compiler_flag(-mssse3)` reports "unsupported" — rcc
warns-and-ignores unrecognized `-m*` flags rather than erroring like
GCC/Clang would, which is what `check_c_compiler_flag` relies on to
detect support. This is a separate, deliberate rcc driver behavior
(silently accepting instruction-set hints it doesn't need), not
addressed this session. TCC 118/118, Unit 170/170, Torture 3605/3609
(100%), Dg-error 34/34, Link 4/4.

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

### Fixed (2026-08-08, httpparser session)

- **`-funroll` (opt.c) aliased duplicated labels across unrolled loop
  copies** — a `for` loop with a constant trip count and a `goto`/label
  pair in its body (e.g. `for (type_both = 0; type_both < 2;
type_both++) { ...; if (parser.upgrade) goto test; ...; test: ...; }`)
  got unrolled into N independent code copies via `clone_expr()`, a
  shallow per-field AST copy that left every copy's `ND_LABEL` with the
  _same_ `label_name`. codegen.c resolves both `goto` and `&&label` by
  formatting one symbolic name, `.L.label.<enclosing-fn>.<label_name>`
  — since every unrolled copy lives in the same enclosing function, all
  copies collided on that one symbol, so a later copy's `goto` bound to
  an _earlier_ copy's already-emitted address instead of its own,
  not-yet-emitted one. Taking that branch resumed execution mid an
  earlier copy with the later copy's live registers/stack state, which
  fed back into the earlier copy's own loop-continue path — corrupting
  the _enclosing_ scan's own induction variables and turning a bounded
  double loop into one that ran roughly 25x too many iterations before
  the test harness's 60s timeout caught it. Reproducible at any `-O2`+
  `-funroll` build regardless of target OS (identical AST-level bug,
  not codegen-backend-specific). Fixed by collecting the label names a
  loop body defines before unrolling and suffixing each unrolled copy's
  matching `ND_LABEL`/`ND_GOTO`/`ND_LABEL_VAL` nodes with a per-copy
  tag, so each copy's `goto`/`&&label` only ever binds to that same
  copy's own `label:` — labels the body merely jumps to _outside_ the
  loop (e.g. a shared `error:`) are left untouched since only names the
  loop itself defines are collected.
  → unblocks: test_httpparser (`test_scan()`'s `for (type_both =
0; type_both < 2; ...)`, found via the request-scan phase's upgrade
  test messages)

New regression test: `test/test_unroll_label_alias.c` — a bounded,
non-hanging reproduction (an escape-hatch counter breaks out after 20
bounces instead of spinning like the real bug) that fails fast
(`assert`) on the unfixed compiler at `-O2`/`-O3` and passes at every
optimization level once fixed. Full suite verified: TCC 118/118, Unit
tests 171/171 (also verified separately at `-O2`), Torture 3605/3609
(100% of non-skipped), Dg-error 34/34 — identical to baseline, plus
confirmed clean on the mingw and arm64 cross targets.

### Fixed (2026-08-08, test_bash session)

- **`eval_const_expr()`'s `ND_MEMBER` fold treated any _global_ with a
  literal initializer as compile-time-constant forever** (parser.c) — the
  fold's guard was `root_var->is_constexpr || !root_var->is_local`: for
  a plain (non-`const`) global struct/union, `!is_local` is true and a
  static initializer sets `has_init`, so a `global.member` read folded
  to the value baked into the _initializer_ — permanently, for every
  later read in the translation unit — even though an ordinary mutable
  global is written to at runtime throughout the program. Once a branch
  like this folds away, no later write can ever bring it back.
  Found via bash's `struct dstack dstack = { NULL, 0, 0 };` (parse.y):
  `current_delimiter(dstack)` (`dstack.delimiter_depth ? ... : 0`) folded
  to a permanent `0`, so every `if (current_delimiter(dstack) ==
'\'') ...`-style quote-tracking check throughout the hand-written
  parser always took the "not quoted" branch. That broke the alias-
  recursion guard in `shell_getc()` badly enough that alias expansion
  never terminated: `shopt -s expand_aliases; alias command=command;
eval 'command true'` spun forever (`test/third_party/test_bash`'s own
  `tests/alias4.sub`, run by its `make test`). Reproducible at any
  `-O1`+ build (CTFE only runs there); `-O0` passed, which is what made
  this a genuine miscompile rather than a bash logic bug — confirmed by
  diffing rcc-built bash's `alias4.sub` output against real bash's
  (identical after the fix). Root-caused by tracing which `ND_IF`s
  opt.c's dead-branch elimination actually folded during bash's build
  (a temporary `getenv("RCC_TRACE_FOLD")`-gated trace in the fold site)
  and finding `current_delimiter(dstack) == '\''` folding to a constant
  `0` despite `dstack` never being declared `const`.
  Fixed by requiring genuine immutability: `is_constexpr` (real
  `constexpr` declarations and constant-valued compound literals), or a
  non-local variable that is _also_ `const`-qualified — a plain mutable
  global with a literal initializer no longer qualifies.
  → unblocks: test_bash's `run-alias` (was hanging; `run-appendop` and
  `run-arith` also exercise `dstack`-adjacent parser state and pass or
  regress independently, see below)

New regression test: `test/test_const_fold_mutable_global.c` — a
mutable global struct read through an `if` before and after a runtime
write (must observe the write), plus a genuinely-`const` global struct
read the same way (must still constant-fold, guarding against
regressing the legitimate case). Fails on the unfixed compiler at
`-O1`/`-O2`/`-O3` (passes at `-O0`, where CTFE doesn't run at all);
passes at every level once fixed. Full suite verified: 0 failed
(Linux native `--all`, Torture 3605/3609 = 100% of non-skipped); Unit
tests 172/172, also verified separately at `-O2`.

### Confirmed rcc bug, not yet fixed: test_bash `run-arith`/`run-arith-for`

While fixing `run-alias` above, bash's own `make test` (run to
completion for the first time) surfaced a second, unrelated failure
further into the suite: `run-arith` prints wrong numeric results (values
matching neither expected small integers nor an obviously-corrupt
pattern — e.g. `76`, `1070722096`, `4655621`, `140737478876000`) and
`run-arith-for` hangs. Minimal repro (`tests/arith-for.tests`'s own
post-bash-4.2 case):

```sh
for (( i = j = k = 1; i % 9 || (j *= -1, $( ((i%9)) || printf " " >&2; echo 0), k++ <= 10); i += j ))
do printf "$i"; done
```

Real bash prints a 5-cycle zigzag (`12345678 987654321 0123...`) and
halts; rcc-built bash counts `i` up linearly forever (`j *= -1` never
seems to take effect, or `i % 9` never re-evaluates true), never
satisfying the loop's own termination condition.
**Not the same root cause as the `dstack` fix above** — confirmed by
rebuilding `y.tab.c` (parse.y, where the `for ((...))` grammar action
lives) at `-O0` and even `expr.c` + `execute_cmd.c` + `subst.c`
together at `-O0` (arithmetic evaluation, command execution, and
command substitution — the three subsystems this one-liner exercises):
the hang persists either way, so it isn't a CTFE/dead-branch artifact
like `dstack` and needs its own bisection (likely something in how
`$(...)` command substitution's side effects interact with `((...))`'s
comma-operator/short-circuit evaluation, or a register/stack-state bug
specific to that nesting) starting from a `-O0`-vs-`-O1` compare across
the remaining untried files, not yet attempted.

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

1. **Missing x86 intrinsics** (~10 projects remaining) — partially
   fixed (`__builtin_ia32_pause`/`mfence`/`lfence`/`sfence`, SSSE3
   `tmmintrin.h`/`_mm_shuffle_epi8`, see "Fixed (2026-08-07, blosc2
   session)" above)
   - `__v8hi`, `__builtin_shufflevector` (GCC vector ext) → test_blake3, test_brotli, test_ffc, test_fftw, test_libwebp, ...
   - Root: rcc can't parse GCC's `<*mmintrin.h>` headers; these use `__v8hi` types and `__builtin_ia32_*` builtins
   - blosc2 itself now builds/links/tests cleanly (see above); its
     one residual gap is CMake's `check_c_compiler_flag(-mssse3)`
     reporting unsupported (driver behavior, not an intrinsics gap)

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

| test              | notes                                                                                                                                                                       |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| test_bash         | `run-alias` **fixed** — was: `dstack` const-fold bug, see "Fixed (2026-08-08, test_bash session)" above; `run-arith`/`run-arith-for` still broken (separate bug, see below) |
| test_perl         | —                                                                                                                                                                           |
| test_go           | —                                                                                                                                                                           |
| test_nginx        | —                                                                                                                                                                           |
| test_groff        | —                                                                                                                                                                           |
| test_argtable3    | —                                                                                                                                                                           |
| test_httpparser   | **fixed** — was: `-funroll` label-aliasing bug, see "Fixed (2026-08-08, httpparser session)" above                                                                          |
| test_libarchive   | —                                                                                                                                                                           |
| test_liblz4       | —                                                                                                                                                                           |
| test_libpng       | —                                                                                                                                                                           |
| test_libressl     | —                                                                                                                                                                           |
| test_qbe_simplecc | —                                                                                                                                                                           |

---

## Quick Wins (next to fix)

1. **test_mruby crash** — confirmed real rcc bug (stack overflow via
   infinite recursion in error formatting); needs gdb investigation
   with the actual mruby VM environment, see the detailed writeup
   above
2. **Missing x86 intrinsics**: `__v8hi`/`__builtin_shufflevector` GCC
   vector-extension surface — affects ~10 remaining projects (blake3,
   brotli, fftw, libwebp, ...). bearssl and blosc2 unblocked
   (SSSE3/`tmmintrin.h` now available, see "Fixed (2026-08-07, blosc2
   session)" above).
3. **C23 `_BitInt(N)`** — test_cproc, test_c23doku
