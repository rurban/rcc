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
  → unblocks: test_bash's `run-alias` (was hanging)

New regression test: `test/test_const_fold_mutable_global.c` — a
mutable global struct read through an `if` before and after a runtime
write (must observe the write), plus a genuinely-`const` global struct
read the same way (must still constant-fold, guarding against
regressing the legitimate case). Fails on the unfixed compiler at
`-O1`/`-O2`/`-O3` (passes at `-O0`, where CTFE doesn't run at all);
passes at every level once fixed. Full suite verified: 0 failed
(Linux native `--all`, Torture 3605/3609 = 100% of non-skipped); Unit
tests 172/172, also verified separately at `-O2`.

### Fixed (2026-08-08, continued — small-struct return ABI)

- **SysV x86-64 small-aggregate return: rcc always used a hidden return
  pointer for every struct/union return, regardless of size** (codegen.c)
  — `has_hidden_retbuf` / the prologue's `param_index` / `has_retbuf`
  (three near-duplicated classification sites) all treated _any_
  `TY_STRUCT`/`TY_UNION` return type as needing a hidden pointer
  argument. The real ABI only requires that for aggregates >16 bytes (or
  ones that don't classify as all-INTEGER/all-SSE eightbytes); a struct
  ≤16 bytes with no floating member returns in RAX (first eightbyte) and
  RDX (second eightbyte, if >8 bytes) with **no** hidden pointer at all.
  Every rcc-compiled caller and callee agreed with each other (so pure
  rcc-to-rcc code, single- or multi-TU, never showed a symptom), but
  calling any _external_ (non-rcc-compiled) function with this return
  shape silently passed a phantom hidden-pointer argument the real
  callee never expects — shifting every real argument into the wrong
  register — and then read the "return value" out of a buffer the
  callee never wrote to.
  → found via bash's own arithmetic evaluator (`expr.c`), which computes
  every `%`/`/` inside `$(( ))` via glibc's `imaxdiv()` (`intmax_t
quot, rem;`, 16 bytes, all-integer): confirmed in isolation first —
  `div()`/`ldiv()`/`lldiv()`/`imaxdiv()` all returned `{0, 0}`
  regardless of arguments — then traced back to bash: `run-arith`
  printed wrong numbers and `run-arith-for` hung
  (`tests/arith-for.tests`'s post-bash-4.2 case, a `for ((...))` whose
  termination depends on `i % 9`, never terminated once `%` always
  returned 0). Root-caused with a minimal `imaxdiv()`-calling
  reproduction once the earlier `-O0` bisection (which ruled out
  `y.tab.c`/`expr.c`/`execute_cmd.c`/`subst.c` optimization artifacts,
  since this bug fires at _every_ optimization level including `-O0`)
  pointed away from a CTFE-style miscompile and toward a calling-
  convention issue instead. Confirmed cross-architecture: rcc-arm64 has
  the identical bug (AAPCS64 has the same ≤16-byte no-hidden-pointer
  rule via X0:X1) — **only the x86-64 SysV path is fixed this session**;
  ARM64 is untouched (still always hidden-pointer for structs) to avoid
  shipping a half-implemented AAPCS64 register-return path, and is
  tracked as a follow-up below.
  Fixed by classifying struct/union return types ≤16 bytes with no
  float/double/long-double/`_Complex` member anywhere (recursively
  through nested structs/unions/arrays) as GP-register returns, and
  wiring that classification into all three x86-64 SysV decision sites
  plus the actual RAX/RDX value transfer at both the call site
  (`gen_funcall`) and the function definition (`ND_RETURN`). A second,
  subtler bug surfaced while fixing this: the function epilogue already
  saved/restored RAX around `__attribute__((cleanup(...)))` handler
  calls (so a pending scalar return value survives a cleanup handler's
  own function calls, which clobber caller-saved registers) but never
  did the same for RDX — so a >8-byte register-returned struct whose
  local had a cleanup attribute lost its upper eightbyte the moment the
  cleanup handler made any call of its own (regressed the TCC suite's
  `101_cleanup` mid-fix: `test_cleanup3`'s 16-byte `tsti` came back `42
43 0 0` instead of `42 43 44 45`). Fixed by saving/restoring RDX
  around cleanup calls exactly like RAX, gated on the function actually
  returning a >8-byte GP-register struct.
  → unblocks: test*bash's `run-arith`/`run-arith-for` (both now pass),
  plus nine \_other* bash test suites that looked unrelated at first
  glance but turned out to depend on correct internal arithmetic too
  (`run-assoc`, `run-comsub2`, `run-coproc`, `run-extglob`,
  `run-heredoc`, `run-histexpand`, `run-jobs`, `run-lastpipe`,
  `run-nameref` — all diffed against real bash before this fix, all
  match after)

New regression test: `test/test_struct_return_gpregs.c` — calls the
libc div-family functions directly (the real-world trigger) plus a set
of user-defined struct shapes (16-byte all-integer, struct with a
pointer member, nested small struct, through a function pointer, direct
assignment) that must return correctly, alongside a >16-byte struct and
a struct with a float member that must still take the (unchanged)
hidden-pointer path. Fails (`assert` abort) on the unfixed compiler at
every optimization level (including `-O0` — this bug is a calling-
convention defect, not an optimizer artifact); passes at every level
once fixed. Full suite verified: 0 failed (Linux native `--all`,
Torture 3605/3609 = 100% of non-skipped); Unit tests 173/173, also
verified separately at `-O2`. Real-world repro: bash's own `make test`
now runs to completion (previously hung); comparing which of its 86
test scripts diff against a real-bash baseline run in the same sandbox
dropped from 19 to 1 (the one remaining, `run-glob-bracket`, was an
unrelated `-rdynamic`/dynamic-loadable-builtin symbol-visibility
issue — since fixed, see "Fixed: test_bash `run-glob-bracket`" below).

### Fixed rcc bug: ARM64 small-struct return ABI

AAPCS64 has the same "small aggregate returns in registers, no hidden
pointer" rule as x86-64 SysV (≤16 bytes, non-HFA composite types return
raw bits in X0:X1) — rcc-arm64 has the identical always-hidden-pointer
bug fixed above for x86-64 (confirmed: `div()`/`ldiv()`/`lldiv()` all
return `{0, 0}` under `qemu-aarch64` too). Not fixed this session to
avoid shipping a half-verified AAPCS64 register-return path; needs the
same three-site classification change plus X0/X1 value-transfer logic
at the ARM64 call site and `ND_RETURN`, and the equivalent of the RDX
cleanup-preservation fix (X1, if ARM64's epilogue has an analogous
X0-only cleanup save/restore — not yet checked).

### Fixed: test_bash `run-glob-bracket` (`-rdynamic` not implemented)

`tests/run-glob-bracket` failed: `./bash: symbol lookup error:
./strmatch.so: undefined symbol: strmatch`. bash's test loads a
small shared-library "loadable builtin" (`enable -f ./strmatch.so
strmatch`) that calls back into a `strmatch` symbol the main `bash`
binary is expected to export. Root cause: `-rdynamic` (bash's own
`LOCAL_LDFLAGS`) wasn't recognized by rcc's driver at all — it fell
through the generic "ignored unknown option" catch-all (main.c) and
was silently dropped before ever reaching the linker command line, so
neither rcc's native ELF linker nor its GCC fallback ever saw it.
Even where rcc's native ELF linker successfully handles the whole
link (which this bash invocation doesn't reach — an all-`.o`
"link-only" invocation with no freshly-compiled source always falls
through to the GCC fallback today, a separate pre-existing gap, not
addressed here), its own dynamic-symbol-table builder (link_elf.c)
only ever collected an executable's imported (undefined) dynamic
symbols, never its own definitions — real `-rdynamic`/
`--export-dynamic` additionally exports every globally visible
defined symbol so a later `dlopen()`'d object can resolve back into
the main program.
Fixed by: (1) recognizing bare `-rdynamic` and `-Wl,-E`/
`-Wl,--export-dynamic` in main.c, threading a new `opt_export_dynamic`
flag through `rcc_link`/`LinkState` (and still appending the flag
verbatim to the `libs` string so the GCC fallback path also receives
it); (2) extending link_elf.c's existing `-shared` "collect every
global symbol" export path (previously `if (s->opt_shared)`) to also
fire for `opt_export_dynamic` on a plain executable, appending those
symbols to `.dynsym`/`.gnu.version`/`.hash` alongside the imports at
their final patched addresses.
→ unblocks: test_bash's `run-glob-bracket` (rebuilt bash now exports
`strmatch`, matching gcc's `-rdynamic` `.dynsym`; the plugin's dlopen
resolves and the test's output matches `glob-bracket.right`).

New regression test: `test/test-link.sh` case 6 ("-rdynamic dlopen
callback") — a main program built with `-rdynamic` that `dlopen()`s a
separately-built plugin `.so` calling back into a symbol defined in
the main program, mirroring bash's `strmatch.so` shape; fails to even
load (`dlopen`/`dlsym` return NULL) without the fix, passes with it.
Full suite verified: Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, Link tests 5/5 (incl. the new case), 0 failed overall; also
confirmed clean on the mingw and arm64 cross-compile targets (compile
only — `-rdynamic`'s native-ELF-linker effect is Linux-specific).

### Fixed (2026-08-08, continued — assignment-expression-as-lvalue)

- **`gen_addr()`'s `ND_ASSIGN` case computed the address of an
  assignment's target without ever emitting the assignment's store**
  (codegen.c) — `(a = b).member`, `&(a = b)`, and chain assignments
  through a struct RHS (`d = e = a[0]`) all route through `gen_addr()`
  to get an lvalue address; its `ND_ASSIGN` case was simply `return
gen_addr(node->lhs);` — the address of `a`, computed _without_ first
  generating `a = b`'s actual store. Every read through that address
  then saw `a`'s stale/uninitialized prior contents instead of `b`.
  Reproducible with three lines of plain C, no mruby involved:
  `struct { uintptr_t w; } a, b = {42}; uintptr_t r = (a = b).w;`
  prints garbage instead of `42`; `gcc`/`clang` both print `42`
  correctly. One call site already carried a matching comment ("For
  chain assignments (d = e = a[0] = c), use gen() to trigger inner
  assignment evaluation, not gen*addr() which skips it") for the
  struct-assignment RHS case (parser.c:7386-7387) — this was the same
  bug, just unfixed for the `gen_addr()` entry point itself (`.member`
  access and `&`).
  → found via mruby 4.0.0's VM: word-boxed `mrb_value` is `{ uintptr_t
w; }`, and GCC-style macros like `WORDBOX_OBJ_TYPE_P(o,n)
(!mrb_immediate_p(o) && mrb_val_union(o).bp->tt == n)` expand their
  parameter `o` multiple times, so a real call site like
  `mrb_hash_p(kdict = regs[kidx])` (`OP_KARG`'s keyword-argument
  dispatch, `src/vm.c`) re-expands `kdict = regs[kidx]` textually
  several times — this bug meant only the \_first* repetition's address
  computation happened and the store into `kdict` never did, so every
  `mrb_hash_p`/`mrb_hash_key_p` check downstream read `kdict` as
  garbage. Confirmed real (not pre-existing/environment): the previous
  session's investigation of this same test*mruby failure (see below,
  formerly a full `mrbtest` SIGSEGV from infinite recursion in error
  formatting) had already fully fixed the crash's proximate trigger via
  the earlier small-struct-return-ABI session; what remained was this
  bug, surfacing as 5 spurious `ArgumentError`s ("missing keyword: …",
  wrong `Hash#key?`/`#==` results) that `mrbtest` itself catches and
  counts as "Crash" (not an OS-level SIGSEGV, mruby's own VM-level
  exception guard).
  Fixed by actually generating the assignment (`gen(node)`, which
  performs the store) before using its target's address: for
  struct/union/array/complex targets `gen()` already returns the
  destination address directly (reused as-is, matching the existing
  chain-assignment fix's pattern); for scalar targets `gen()` returns
  the assigned \_value*, discarded before re-taking the target's
  address.
  → unblocks: test_mruby's `mrbtest` — `Total: 1686, OK: 1677, KO: 0,
Crash: 0` (was: full SIGSEGV, no summary at all, before the earlier
  struct-return-ABI fix; `Crash: 5` after that fix; `KO: 2` after this
  fix but before the `Math.erf`/`Math.erfc` fix below — `mrbtest` now
  matches gcc-built mruby's own summary exactly, including its 9
  environment-only `Skip`s). See "Fixed (2026-08-08, continued —
  include/math.h erf/erfc)" below for the last 2 failures.

New regression test: `test/test_assign_expr_lvalue.c` — struct member
access on an assignment expression's result, the same assignment used
twice within one `&&`-sequenced expression (the real macro-re-expansion
shape), `&` on a scalar assignment result, and a struct chain
assignment; all cross-checked against gcc. Full suite verified: Torture
3605/3609 (100% of non-skipped), Dg-error 34/34, Unit tests 176/176,
Link tests 5/5, 0 failed overall; confirmed clean (test PASSes) on the
mingw and arm64 cross-compile targets.

### Fixed (2026-08-08, continued — include/math.h erf/erfc)

- **`include/math.h` (rcc's own bundled header, used in preference to
  the system `<math.h>` the same way `emmintrin.h`/`tmmintrin.h` are)
  had no declaration for `erf()`/`erfc()` at all** — most of ISO C99's
  `<math.h>` surface was there (`sin`, `cos`, `tgamma`... no, not even
  that far — just the commonly-used subset), but `erf`/`erfc` were
  simply missing. Calling an undeclared external function silently
  falls back to an implicit `int` return type (K&R legacy behavior,
  accepted with no diagnostic even under `-W`): `double y = erf(x);`
  treated the call's return value as arriving in `RAX` (the
  integer-return register) and converted it to `double` via
  `cvtsi2sd`, instead of reading the real `double` result glibc placed
  in `XMM0` per the SysV ABI. The call itself succeeded and glibc
  computed the mathematically correct value; only the _caller's_
  interpretation of the return register was wrong — confirmed by
  disassembling the call site (`call erf@plt` immediately followed by
  `mov %rax,%r10; cvtsi2sd %r10d,%xmm0`, versus `sin`/`cos`/`sqrt`/
  `exp`/`log`, all present in `include/math.h` and all reading `xmm0`
  correctly).
  Reproducible with three lines of plain C, no mruby involved:
  `double y = erf(1.0);` prints `1072693248` (a plausible-_looking_
  large-integer bit pattern, not obviously garbage — which is what
  made this miscompile easy to spot in test _output_ but easy to miss
  by code inspection) instead of `~0.8427`.
  Fixed by adding `double erf(double);`/`double erfc(double);` to
  `include/math.h`.
  → unblocks: test_mruby's `mrbtest` — `Total: 1686, OK: 1677, KO: 0,
Crash: 0, Skip: 9`, now byte-for-byte matching the reference
  gcc-built mruby's own summary. **`test_mruby` is fully fixed.**

New regression test: `test/test_math_erf.c` — checks `erf`/`erfc`
against their true mathematical values (tight tolerance, not just "in
some plausible range" — a wrong-but-plausible-looking value like the
one this bug actually produced would trivially pass a loose bounds
check) plus the `erf(x) + erfc(x) == 1` identity. Full suite verified:
Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Unit tests
177/177, Link tests 5/5, 0 failed overall; confirmed clean (test
PASSes) on the mingw and arm64 cross-compile targets.

### Historical: the original test_mruby crash writeup (superseded above)

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

Investigated but NOT reproduced in isolation (at the time):

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
  (Superseded: this build's own `MRB_WORD_BOXING` default, not
  `boxing_no.h`/`MRB_NO_BOXING`, was actually active — see the fix
  above.)

- `test/third_party/results.txt` — tab-separated: `rc\ttest_name\tduration`
- `test/third_party/logs/test_*.log` — per-project build + test output
- `test/third_party/test_<name>/` — failing project dirs (passing dirs auto-deleted)

Re-run a single test:
CC=$(pwd)/rcc bash test/linux*thirdparty.bash test*<name>

---

## rc=1 — Runtime Failures (builds OK, test fails)

| test             | symptom                                                                                                                                                                                                                  |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| test_lua         | db.lua:83 assertion: debug.getinfo(f).short_src                                                                                                                                                                          |
| test_mruby       | **fixed** — was: assignment-expr-as-lvalue bug + missing `erf`/`erfc` declarations, see "Fixed (2026-08-08, continued — ...)" sections above; `Total: 1686, OK: 1677, KO: 0, Crash: 0` (matches gcc-built mruby exactly) |
| test_curl        | **fixed** — was: configure "compiler does not halt on prototype mismatch"                                                                                                                                                |
| test_c23doku     | C23 \_BitInt(N) not supported                                                                                                                                                                                            |
| test_c3          | CMake: missing LLD_COFF                                                                                                                                                                                                  |
| test_coremarkpro | benchmark runner can't find perf logs                                                                                                                                                                                    |
| test_box3d       | C++ binary (g++ compiled, not rcc)                                                                                                                                                                                       |
| test_glib        | —                                                                                                                                                                                                                        |
| test_got         | configure: missing libbsd-overlay                                                                                                                                                                                        |
| test_ksh93       | —                                                                                                                                                                                                                        |
| test_libgmp      | configure: cannot determine 32-bit word directive                                                                                                                                                                        |
| test_muon        | muon self-tests (some pass, some fail)                                                                                                                                                                                   |
| test_neovim      | —                                                                                                                                                                                                                        |
| test_nob         | git checkout only (build not reached?)                                                                                                                                                                                   |
| test_rsync       | —                                                                                                                                                                                                                        |
| test_samba       | —                                                                                                                                                                                                                        |
| test_scrapscript | rcc compile fails (exit 1) during Python test harness                                                                                                                                                                    |
| test_tcpdump     | —                                                                                                                                                                                                                        |

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

### Fixed (2026-08-08, shufflevector/SSE2-baseline session)

- **`__builtin_shufflevector` unimplemented** (parser.c) — a Clang
  builtin (`vec1, vec2, i0, i1, ..., iN-1`, all indices compile-time
  constants, output lane count = however many indices were given) that
  GCC 15's own `<avxintrin.h>`/`<avx2intrin.h>` now also use
  unconditionally in several `_mm_reduce_*`/`_mm256_reduce_*` function
  bodies. rcc has no bundled AVX/AVX2 headers, so any project that
  merely does `#include <immintrin.h>` (regardless of whether it calls
  any AVX function) falls through to the real system headers and hits
  this — the whole TU failed to parse ("expected an expression") at
  every one of those reduce-macro call sites, and a syntax error in
  the middle of a declaration list cascaded into spurious errors on
  every subsequent statement.
  Implemented generically at parse time (like the existing
  `__builtin_shuffle`, but with immediate indices instead of a runtime
  mask vector, and independent output length): binds both operand
  vectors to temps once, then for each index either gathers
  `vec1[idx]` (idx < N1), `vec2[idx - N1]` (N1 <= idx < N1+N2), or an
  arbitrary lane (idx < 0, Clang's "don't care" convention) into a
  freshly-sized result vector built via the existing
  `make_vector_type()`. Portable — no architecture-specific codegen,
  works identically on ARM64.
  → unblocks: test_blake3's `blake3_dispatch.c` (parses past the
  `<immintrin.h>` chain; blake3 itself still needs real AVX-512
  codegen it doesn't get from rcc, see below)
- **Large baseline-SSE2 gap in `include/emmintrin.h`** — once
  `__builtin_shufflevector` no longer aborted the parse early, GCC's
  own `avx2intrin.h` reduce macros went on to call `_mm_min_epi16`/
  `_mm_max_epi16` by name, which don't exist in rcc's _own_
  (deliberately minimal, hand-written) `emmintrin.h` — found to be
  missing an entire tier of plain SSE2 (no SSSE3+) intrinsics that
  real programs call directly too: `_mm_min_epi16`/`_mm_max_epi16`/
  `_mm_min_epu8`/`_mm_max_epu8`, the saturating `_mm_adds_`/`_mm_subs_`
  family (epi8/epi16/epu8/epu16), `_mm_avg_epu8`/`_mm_avg_epu16`,
  `_mm_mulhi_epi16`/`_mm_mulhi_epu16`, `_mm_madd_epi16`,
  `_mm_sad_epu8`, and the whole-_register_ byte shifts
  `_mm_srli_si128`/`_mm_slli_si128`/`_mm_bsrli_si128`/`_mm_bslli_si128`
  (distinct from the per-lane `_mm_s{r,l}li_epi{16,32,64}` that did
  exist). All implemented as plain lane-wise C loops (matching the
  file's existing style), every result cross-checked against real
  gcc's `<emmintrin.h>` output.
  → unblocks: test_libwebp (`sharpyuv_sse2.c` calls `_mm_max_epi16`/
  `_mm_min_epi16`/`_mm_madd_epi16` directly; `dec_sse2.c` calls
  `_mm_srli_si128`/`_mm_slli_si128` directly) — now builds, links, and
  its `cwebp`/`dwebp` tools successfully decode a real `.webp` image

New regression tests: `test/test_builtin_shufflevector.c` (portable,
no arch guard — exercises cross-operand selection, output-length
narrowing, single-vector reversal, and a non-`short` element type),
`test/test_sse2_minmax_madd_sat.c` (x86-only guarded like
`test_ia32_pause.c`'s convention). Full suite verified: Torture
3605/3609 (100% of non-skipped), Dg-error 34/34, Unit tests 175/175,
Link tests 5/5, 0 failed overall; confirmed clean (both new tests
PASS) on the mingw and arm64 cross-compile targets.

**Still blocked** (much larger undertakings, not attempted this
session): test*blake3 needs real AVX-512 (`avx512fintrin.h` pulls in
`__mmask8`/`_CMP_EQ_OQ`-style predicates and hundreds of
`\_\_builtin_ia32*\*` AVX-512 builtins rcc has no codegen for at all);
test_brotli needs real AVX2 codegen similarly; test_fftw's failure
(`fftw3.h:483`, `\_\_float128`/`FFTW_DEFINE_API`quad-precision) is
unrelated to SIMD entirely. A genuine fix for blake3/brotli would mean
rcc shipping its own`<immintrin.h>`/`<avxintrin.h>`/`<avx2intrin.h>`(the way it already does for`<emmintrin.h>`/`<tmmintrin.h>`) instead
of falling through to GCC's real, AVX-512-chaining system headers —
a multi-session effort, not a quick win.

### Needs fixing

1. **C23 `_BitInt(N)`** — test_cproc, test_c23doku
   - `_BitInt(total * 3)` → "expected specific operator"

2. **lib/tempname.c pattern** (now partially fixed)
   - `SIZE_WIDTH` undeclared in test_diffutils (project-specific macro, not stdint)

3. **Object file passed as source** — test_heatshrink
   - `.os` file compiled as C source (build system issue, not rcc)

4. **Link failures (environment, not rcc)**: test_file, test_libgc, test_libjansson, ...
   - Missing system libs: libseccomp, libzstd, etc.

---

## rc=124 — Timeouts

| test              | notes                                                                                                                                                                                                                 |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| test_bash         | **fixed** — `dstack` const-fold bug + small-struct return ABI bug + `-rdynamic` not implemented, see "Fixed (2026-08-08, ...)" sections above; own `make test` now runs to completion, `run-glob-bracket` also passes |
| test_perl         | —                                                                                                                                                                                                                     |
| test_go           | —                                                                                                                                                                                                                     |
| test_nginx        | —                                                                                                                                                                                                                     |
| test_groff        | —                                                                                                                                                                                                                     |
| test_argtable3    | —                                                                                                                                                                                                                     |
| test_httpparser   | **fixed** — was: `-funroll` label-aliasing bug, see "Fixed (2026-08-08, httpparser session)" above                                                                                                                    |
| test_libarchive   | —                                                                                                                                                                                                                     |
| test_liblz4       | —                                                                                                                                                                                                                     |
| test_libpng       | —                                                                                                                                                                                                                     |
| test_libressl     | —                                                                                                                                                                                                                     |
| test_qbe_simplecc | —                                                                                                                                                                                                                     |

---

## Quick Wins (next to fix)

1. **AVX2/AVX-512 codegen** — test_blake3 (AVX-512), test_brotli (AVX2)
   need rcc to ship its own `<immintrin.h>`/`<avxintrin.h>`/
   `<avx2intrin.h>` (and, for blake3, AVX-512 mask-register codegen)
   instead of falling through to GCC's real system headers; a
   multi-session effort, see "Fixed (2026-08-08,
   shufflevector/SSE2-baseline session)" above for what's already
   covered (`__builtin_shufflevector`, the missing baseline-SSE2 tier —
   both unblocked test_libwebp).
2. **C23 `_BitInt(N)`** — test_cproc, test_c23doku
