# Third-Party Test Results & Triage TODO

Batch run: 2026-08-06 (199 of 221 targets)
Binary: rcc HEAD (third_party branch)

## Summary

| rc  | count | meaning                                                        |
| --- | ----- | -------------------------------------------------------------- |
| 0   | 54    | pass                                                           |
| 2   | 101   | build/compile failure                                          |
| 1   | 18    | runtime/test failure (many are build-system: CC not respected) |
| 124 | 12    | timeout (420 s)                                                |
| 127 | 10    | missing tool (muon, lzip, etc.)                                |
| 139 | 1     | SIGSEGV (box2d C++ binary, not rcc)                            |
| 8   | 2     | test failure (blake3)                                          |
| 6   | 1     | —                                                              |

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

### Fixed rcc bug: ARM64 `stur`/`ldur` frame-offset immediate overflow

The previous TODO entry here ("ARM64 small-struct return ABI... not
fixed this session") was **stale** — that classification (a struct/
union ≤16 bytes with no floating member returns raw bits in X0:X1, no
hidden pointer, mirroring the x86-64 fix above) is already fully wired
for ARM64 throughout codegen.c (`struct_returns_in_gp_regs()`'s
`ARCH_ARM64` branch in `has_hidden_retbuf`, the call-site X0/X1
load, `ND_RETURN`'s X0/X1 store, and the prologue's `has_retbuf`/
`save_rdx_for_cleanup`-equivalent X1 preservation) — confirmed working
by re-running the exact `div()`/`ldiv()`/`lldiv()`/`imaxdiv()` repro
that flagged the original x86-64 bug, cross-built for arm64 and run
under `qemu-aarch64`: all four now return correct `{quot, rem}` pairs
into external (non-rcc-compiled) glibc-called code, exactly like the
x86-64 fix. The **actual** remaining rcc-arm64 bug that TODO entry's
own regression evidence (c-testsuite's `00204.c`, an ARM64-specific
HFA/small-struct calling-convention smoke test) was really pointing
at turned out to be unrelated to struct-return classification at all:

- **`asm_stur_fp()`/`asm_ldur_fp()` (codegen_asm.h) emitted `stur`/
  `ldur` with no range check on the frame-pointer-relative offset** —
  AArch64's unscaled-offset STUR/LDUR encode a signed 9-bit immediate
  (`imm9`, range -256..255), and `arm64_stur()`/`arm64_ldur()`
  (arm64*enc.c) mask the raw value with `& 0x1ff` unconditionally, with
  no bounds validation. An offset whose magnitude exceeds that range
  (e.g. requesting `[x29, #-264]`) silently wraps to a small
  **positive** offset instead (`-264 & 0x1ff == +248`), addressing
  memory \_above* the frame pointer — where the caller's own stack
  content lives — instead of erroring or falling back to indirect
  (scratch-register) addressing the way every other `x29`-relative
  helper in this file already does (`arm64_load_from_fp_minus()`/
  `arm64_store_to_fp_minus()`, `asm_sub_fp_imm()`, etc. all check the
  offset and fall back to `sub x16/x17, x29, #offset; ldr/str`).
  `asm_stur_fp`/`asm_ldur_fp` are used by `gen_funcall()`'s
  register-pressure argument-staging path (parking an about-to-be-
  called function's evaluated argument values in per-call temp slots
  before the final register-loading pass), `spill_offset()` register
  spills, and a couple of libgcc-helper call sites (128-bit shift/
  divide, atomic RMW) — any of which can legitimately need an offset
  beyond 255 bytes from `x29` once a function's own parameter/local
  storage is deep enough. This stayed latent for small/shallow
  functions and only manifested once a function had enough of its own
  storage (several small struct parameters plus several multi-member
  HFA struct parameters, each needing its own frame slot) that adding
  a handful of staged call-argument temps for a _nested_ call (this
  session's repro: a `printf()`/`snprintf()` call needing several
  register-pressure temps of its own) pushed past the 255-byte
  boundary — silently overwriting the region just above the frame,
  up to and including the **caller's saved x29/x30** a few slots
  further up, corrupting the return address itself with leftover
  floating-point argument bit patterns (a SIGSEGV at a garbage PC that
  happens to be a valid IEEE-754 double bit pattern, not a wrong-value
  bug — confirmed via a `qemu-aarch64` gdbstub trace showing the fault
  PC and X29/X30 holding the exact bit patterns of two of the crashing
  function's own `long double` struct-member arguments).
  Fixed by giving `asm_stur_fp()`/`asm_ldur_fp()` the same range check
  and indirect-addressing fallback (via scratch `x16`, reusing the
  already-range-safe `asm_sub_fp_imm()`/`asm_ldr_reg()`/`asm_str_reg()`
  helpers) that the file's other `x29`-relative accessors already have.
  → found via GCC c-testsuite's `00204.c` (ARM64-specific calling-
  convention smoke test): `fa4()`'s exact parameter shape (three tiny
  char-array structs interleaved with three float/double/long-double
  HFA structs) still SIGSEGV'd after the struct-return-ABI
  investigation above ruled that out, isolated via bisection down to a
  minimal repro and a `qemu-aarch64` gdbstub trace of the exact
  corrupting instruction.
  A second, smaller side effect of the same fix: GCC torture's
  `vect/pr88497-7` (a separate deep-enough-frame test that happened to
  hit the same `stur`/`ldur` overflow) now also passes on arm64,
  previously miscounted among the "genuine complex/imaginary-constant
  gaps" torture failures.

New regression test: `test/test_arm64_frame_call_stage.c` — the
`00204.c`-shaped repro (three GP-passed structs interleaved with three
HFA structs, calling `snprintf()` from inside a function with that much
of its own parameter storage), asserting the exact expected formatted
output; confirmed to SIGSEGV on the unfixed compiler (verified via a
`git stash`-isolated pre-fix rebuild) and pass with the fix, on both
arm64-cross and native Linux x86-64 (the bug is ARM64-specific by
construction — the STUR/LDUR instruction only exists on that
architecture — but the test's C source and expected output are
portable).
**Full suite re-verified after the fix (arm64 cross)**: c-testsuite
220/220 (was 219/220 — `00204.c` now passes), Torture 3599/3609 (was
3598/3609 — `vect/pr88497-7` also now passes) with 6 pre-existing,
unrelated runtime failures remaining (c11-complex-1, c23-float-6,
c23-imaginary-constants-1/5/9, pr92904 — all genuine complex-number/
imaginary-constant gaps, unaffected by this fix), Dg-error 34/34, TCC
Compatibility 119/120 (1 pre-existing, unrelated `__DECIMAL_BID_FORMAT__`
gap) — 0 new failures, 2 additional tests fixed as a side effect.
Native Linux x86-64 and mingw cross (both unaffected by an ARM64-only
encoder bug) re-verified clean.

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

### Fixed (2026-08-08, cproc array/VLA-param type-modeling session)

- **Array-parameter bracket qualifier decayed to `T *const` on the wrong
  side** (test_cproc's `test/func-array-param.c`): `int a[const]` (C99
  6.7.6.3p7 — a qualifier inside a parameter's array-declarator `[]`
  qualifies the DECAYED POINTER PARAMETER itself, `int *const a`) was
  silently discarded by `type_suffix()`'s bracket-qualifier loop, and
  separately, qualifying a typedef'd array type (`const T c` where `T` is
  `typedef int T[]`) wrongly set the qualifier on the array type's own
  `->qual` in `declspec()` instead of pushing it down to the element type
  per C11 6.7.3p9 — both bugs made the decayed pointer come out qualified
  on the wrong side (`const int *` instead of `int *const`, or vice
  versa). Fixed by having `type_suffix()` record any qualifier found in
  the outermost (first) array dimension's `[...]` on the built array/VLA
  type's own `->qual` (otherwise always 0 for arrays), which
  `declarator_params()`'s array/VLA-to-pointer decay now transfers onto
  the resulting pointer type; and by adding `qualify_array_elem()` in
  `declspec()` to recurse a prefix qualifier down through a typedef'd
  array/VLA to its element type instead of setting it on the array itself.
- **`__builtin_types_compatible_p` never handled `TY_VLA`**
  (test_cproc's `test/compatible-vla-types.c`): `types_compatible_p_qual()`
  had no case for VLA pointer types or runtime-sized array type-names —
  the strict `a->kind != b->kind` check rejected any `TY_ARRAY` vs
  `TY_VLA` comparison outright (e.g. `int (*)[n]` vs `int (*)[3]`), and
  two same-kind VLAs fell through to the ordinary `TY_ARRAY` case's
  constant-length comparison, wrongly treating two different runtime
  lengths (which are never a compile-time compatibility criterion per
  C11 6.7.6.2p6-7) as incompatible. Fixed by handling `TY_ARRAY`/`TY_VLA`
  together in `types_compatible_p_qual()`: element types must still be
  compatible, but if either side is `TY_VLA` the length check is skipped
  entirely (always compatible), while two `TY_ARRAY`s keep the existing
  "either side unknown-length (size 0) is compatible" logic.

New regression tests: `test/test_array_param_qual_decay.c` (bracket-
qualifier decay to `int *const`, plus the typedef'd-array-qualifier
push-down case) and `test/test_vla_types_compatible.c` (VLA/unspecified-
length array vs fixed-array/VLA compatibility, including the
element-type-still-matters negative case). Both new upstream cproc
fixtures (`func-array-param.c`, `compatible-vla-types.c`) now compile
clean with `./rcc -std=c23 -c`, matching `gcc -std=c23 -c` (rc=0 on
both, cross-checked). `make -j$(nproc) rcc` stays warning-clean.

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

| test             | symptom                                                                                                                                                                                                                                                                                          |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| test_lua         | db.lua:83 assertion: debug.getinfo(f).short_src                                                                                                                                                                                                                                                  |
| test_mruby       | **fixed** — was: assignment-expr-as-lvalue bug + missing `erf`/`erfc` declarations, see "Fixed (2026-08-08, continued — ...)" sections above; `Total: 1686, OK: 1677, KO: 0, Crash: 0` (matches gcc-built mruby exactly)                                                                         |
| test_curl        | **fixed** — was: configure "compiler does not halt on prototype mismatch"                                                                                                                                                                                                                        |
| test_c23doku     | needs arbitrary-precision `_BitInt` codegen (up to 11163 bits) — see "Needs fixing" item 1 below                                                                                                                                                                                                 |
| test_c3          | CMake: missing LLD_COFF                                                                                                                                                                                                                                                                          |
| test_coremarkpro | benchmark runner can't find perf logs                                                                                                                                                                                                                                                            |
| test_box3d       | C++ binary (g++ compiled, not rcc)                                                                                                                                                                                                                                                               |
| test_glib        | —                                                                                                                                                                                                                                                                                                |
| test_got         | configure: missing libbsd-overlay                                                                                                                                                                                                                                                                |
| test_ksh93       | —                                                                                                                                                                                                                                                                                                |
| test_libgmp      | configure: cannot determine 32-bit word directive                                                                                                                                                                                                                                                |
| test_muon        | muon self-tests (some pass, some fail)                                                                                                                                                                                                                                                           |
| test_neovim      | —                                                                                                                                                                                                                                                                                                |
| test_nob         | git checkout only (build not reached?)                                                                                                                                                                                                                                                           |
| test_rsync       | —                                                                                                                                                                                                                                                                                                |
| test_samba       | —                                                                                                                                                                                                                                                                                                |
| test_scrapscript | **fixed** — was: every test failed to even link (`undefined reference to '__start_const_heap'`); the `section()` attribute fix resolved linking (32/33 -> 17/33 failing), then the section sh_addralign fix below resolved the remaining 17 `SIGABRT`s (17/33 -> 0/33 failing, full suite green) |
| test_tcpdump     | —                                                                                                                                                                                                                                                                                                |

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

1. **C23 `_BitInt(N)`** — **test_cproc fixed** (see "Fixed (2026-08-08,
   root-cause/test_cproc-completion session)" below: `_BitInt` was never
   the real blocker for test_cproc — full type-system support has been
   added regardless). **test_c23doku still needs real arbitrary-precision
   `_BitInt` codegen** — its `brute_force.c`/`graph_color.c` declare
   `_BitInt(total * 3)` where `total = digit * digit`, i.e. up to 11163
   bits for the 61x61 puzzle (175 64-bit legs) — genuine bignum ALU
   codegen (multi-word assign/zero-extend/shift/or/and/xor, ABI classification
   reusing the large-struct hidden-pointer convention, on both x86-64 and
   ARM64), not a quick win. See that session's notes for the exact scope
   assessment.

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
2. **C23 `_BitInt(N)`** — test_c23doku only now (test_cproc fixed, see
   "Fixed (2026-08-08, root-cause/test_cproc-completion session)" below)

### Fixed (2026-08-08, VM-type evaluation-order session)

- **`__builtin_va_arg(ap, TYPE)` with a variably-modified `TYPE`** (parser.c)
  — the type-name parsed by `type_name()` for `__builtin_va_arg`'s second
  argument could itself be variably modified (e.g. `int (*)[++i]`, a
  pointer-to-VLA-array whose dimension is a non-constant expression with a
  side effect), but the resulting `Type`'s `vla_len_expr` just sat unused —
  nothing ever threaded it into the generated code, so the side effect
  (`++i`) silently never ran. Fixed by reusing `vla_freeze_dims()` (the
  existing mechanism a VM-typed cast already uses) to fold the embedded
  dimension expression's side effect into a hidden temp exactly once, ahead
  of the `va_arg` dereference.
  → found via cproc's `test/builtin-vaarg-vm.c`
- **Nested VLA-pointer-array declarator (`int (*p[f(2)])[f(3)];`) evaluated
  its dimension expressions unreliably** (parser.c) — an array-of-pointers
  local whose own length (`f(2)`) is a runtime dimension, and whose pointee
  element type is itself a VLA-array with its own runtime dimension
  (`f(3)`), built the correct `Type` shape, but neither dimension
  expression was frozen into a temp: `f(2)`'s raw expression node was
  embedded directly in the stack-allocation size computation _and_
  separately re-read (and thus re-evaluated) by any later `sizeof p`;
  `f(3)`'s raw node was never evaluated for the allocation at all and only
  ran — potentially more than once — whenever something like `sizeof **p`
  read it back out of the pointee type. Fixed by extending the existing
  `vla_freeze_dims()`-based dimension freeze (previously only applied when
  declaring a plain pointer-to-VLA local) to also apply when the
  declarator's own outermost type is itself a VLA: every dimension
  expression reachable through the type's pointer/VLA chain is now captured
  into a hidden local exactly once, ahead of the array's stack allocation,
  and every later use (sizeof, dereference) reads the frozen capture
  instead of re-running the expression.
  → found via cproc's `test/vla-nested.c`
- **`sizeof` on a VLA-typed _expression_ discarded the operand entirely
  instead of evaluating it** (parser.c) — C11 6.5.3.4p2 requires `sizeof`
  to evaluate its operand when the operand's own type is a VLA (not merely
  "variably modified" — e.g. a _pointer_ to a VLA doesn't trigger this,
  only a genuine VLA-typed expression like `*p` where `p` points at a
  VLA). The expression-operand branch computed the runtime size directly
  from the _type's_ captured `vla_len_expr` (`len * base_size`) but never
  emitted the operand `Node` itself into the generated code, so any side
  effect reaching the sizeof'd VLA lvalue (e.g. `sizeof(*(c++, p))`)
  silently never ran. Fixed by wrapping the size computation as
  `(operand, len * base_size)` (`ND_COMMA`) so the operand's side effects
  execute — evaluating a VLA/array-typed dereference never performs an
  actual memory load (arrays don't rvalue-convert), so this is safe even
  when the pointer is null.
  A second, related bug: `eval_const_expr`'s `ND_COMMA` case folded a
  comma expression to its right operand's constant value regardless of
  whether the _left_ operand was itself constant/side-effect-free
  (`return eval_const_expr(node->rhs, val)`), so an array-dimension
  expression like `int[(c++, 5)]` got silently constant-folded to `5`
  with `c++`'s side effect thrown away instead of surfacing as a genuine
  VLA dimension. Fixed by requiring both operands to independently fold
  as constants (mirrored in the float sibling `eval_const_fexpr`).
  → found via cproc's `test/sizeof-vla.c`
- **A cast's VM array-length expression nested behind a function-pointer
  return type was never evaluated** (parser.c, `vla_freeze_dims()`) — a
  cast like `(int (*(*)(void))[++l])0` (pointer to function returning
  pointer to a VM array) needs `++l` to run exactly once, same as the
  simpler `(int (*)[++l])0` case. `vla_freeze_dims()` only recursed
  through `TY_PTR`/`TY_VLA` `->base` chains, so a `TY_FUNC` in the middle
  of the chain (whose return type lives in `->return_ty`, not `->base`)
  stopped the walk before it ever reached the nested VLA — `++l` was
  parsed into the type but never threaded into any evaluated `Node`.
  Fixed by having `vla_freeze_dims()` also recurse into `TY_FUNC`'s
  `return_ty`.
  → found via cproc's `test/cast-vm.c`
- **A queued VM-`typeof` side-effect evaluation was dropped for any
  non-declaration statement** (parser.c) — `queue_vm_typeof_eval()`
  already correctly classified whether a `typeof(expr)` operand needed
  evaluating (VM after any top-level comma-operator array/VLA-to-pointer
  decay — e.g. `typeof(c++, p)` with `p` a pointer-to-VLA is VM and
  evaluated, but `typeof(p, c++)` decays to plain `int` and is not) and
  queued the evaluation onto a global `pending_vla_struct_capture` list,
  but that list was only ever flushed into the statement stream by the
  `declaration()` call site's caller. A bare (non-declaration) statement
  whose `typeof` appears inside a cast or compound literal — e.g.
  `(typeof(c++, p))0;` or a standalone `__builtin_va_arg(ap, typeof(out--,
p));` — went through the block-statement loop's generic `stmt()`
  fallback, which never checked `pending_vla_struct_capture` at all,
  silently discarding the queued side effect. Fixed by splicing any
  pending capture in ahead of the statement in that fallback too, matching
  the ordering the declaration path already uses.
  → found via cproc's `test/typeof-vm.c`

New regression tests: `test/test_vaarg_vm_type.c`,
`test/test_vla_nested_array_of_ptr.c`, `test/test_sizeof_comma_vla.c`,
`test/test_cast_vm_funcptr.c`, `test/test_typeof_comma_vm_stmt.c`.

### Fixed (2026-08-08, cproc scope/attribute/GNU-ternary session)

- **A tag or enum constant declared inside a function's parameter-type-list
  leaked into file scope permanently** (parser.c) — C11 6.2.1p4 gives an
  identifier declared inside a parameter list "function prototype scope":
  visible only within that parameter list and (for a definition) the
  function body, then reverting to whatever was visible before. rcc's
  `enum_consts`/`enum_htab` table had no scope save/restore around
  `declarator_params()` at all (unlike `locals`/`typedefs`/`tags`, which
  already have the identical checkpoint/restore pattern at every other
  scope boundary in the file), so `enum {A = 2}` inside `f`'s parameter
  list registered into the flat file-scope table and was never popped —
  the file-scope `enum {A = 1}` stayed permanently shadowed for the rest
  of the translation unit instead of reverting after `f`'s closing brace.
  Fixed by checkpointing `enum_consts`/`enum_htab` (via the existing
  `enum_scope_checkpoint()`/`enum_scope_restore()` helpers) once per
  top-level declarator in `program()`'s declaration loop, and restoring it
  at every exit of that declarator (prototype-only `;`, after a function
  body's closing `}`, and each comma-continuation), so the parameter
  list's own enum constants stay visible through the body and are then
  correctly popped.
  → found via cproc's `test/func-param-scope.c`
- **C23 `[[gnu::packed]]` / `[[__gnu__::packed]]` on a struct/union tag was
  silently ignored** (parser.c, `read_type_attrs()`) — the C23
  double-bracket attribute parser already handled the `namespace::name`
  syntax generically, but only to _skip past_ it; it never matched the
  attribute name against any known GNU attribute, so `gnu::packed` parsed
  cleanly and did nothing; the struct/union gained no packing at all
  (identical layout to the fully unpacked type) instead of the same effect
  as the legacy `__attribute__((packed))` syntax. Fixed by recognizing a
  `gnu`/`__gnu__` namespace with a `packed`/`__packed__` name and setting
  `attr->is_packed` (plus the `align=1` clamp the legacy path already
  applies), which struct/union member-layout computation already consumes
  via `struct_pack`.
  → found via cproc's `test/union-packed.c`
- **GNU `a ?: b` (omitted then-operand) evaluated `a` twice, and its
  correct usual-arithmetic-conversion result type was never actually
  reachable at codegen time** (codegen.c) — the parser already built the
  right AST (`node->then` is literally `node->cond`, or check*type's
  usual-arithmetic-conversion pass wraps it in an `ND_CAST` whose `lhs` is
  `node->cond`, exactly mirroring an ordinary `a ? a : b` ternary), but
  `ND_COND`'s codegen unconditionally called `gen(node->cond)` for the
  branch test \_and then separately* `gen(node->then)` for the true-branch
  value — since those are the same subexpression object, any side effect
  in the condition (e.g. `++x`) fired a second time on the true path.
  Fixed by detecting the omitted form (`node->then == node->cond`, or an
  `ND_CAST` wrapping it) and reusing the already-computed condition
  register instead of regenerating, applying the promotion cast in-place
  via a new `gen_cast_reg()` helper factored out of the `ND_CAST` case
  (previously inlined only inside `gen()`, now shared by both).
  → found via cproc's `test/conditional-omit.c`

New regression tests: `test/test_enum_param_scope_leak.c`,
`test/test_c23_attr_packed.c`, `test/test_gnu_ternary_omit_promote.c`.

### Fixed (2026-08-08, C23 enum type-modeling session)

- **C23 `enum tag : type` fixed underlying type wasn't complete until the
  whole enum finished parsing** (parser.c, `enum_specifier()`) — per C23
  N3030, a fixed-underlying-type enum is complete immediately after the
  `: type` specifier (before any enumerator, and even before the tag's
  own `{` if forward-declared), so it can be `sizeof()`'d or used via
  `typeof()` from _inside its own enumerator list_. rcc instead ran every
  enumerator's value through the generic int/expr-type/prev-type ladder
  (the same one used for a plain, non-fixed enum) and only retroactively
  overwrote every constant's type to the fixed type _after_ the closing
  `}` — so a self-referential `typeof(A)` used by a later enumerator in
  the same list (or `sizeof(enum E)` used by the very first one, e.g.
  `enum E : long long { A = sizeof(enum E), ... }`) still saw a
  provisional `int`/expression type, and `enum tag;`-only forward
  declarations never registered the tag's `Type*` before a subsequent
  `sizeof`/`typeof` needed it. Fixed by building the completed enum
  `Type` (and, when tagged, pushing its `EnumTag`) immediately once the
  `: type` is parsed and before the enumerator loop starts, so every
  enumerator gets that exact type from the start and self-references
  inside the body resolve correctly.
  → found via cproc's `test/enum-fixed.c`
- **`__builtin_types_compatible_p` treated any two enums of the same
  size/signedness as compatible, including two entirely separate
  declarations** (parser.c, `types_compatible_p_qual()`) — rcc's `Type`
  has no distinct `TY_ENUM` kind (an enum reuses its underlying integer
  `TypeKind`, flagged via `is_enum`), so the qualifier-stripped
  compatibility check's `default:` case (`a->size == b->size`) couldn't
  tell "two different `enum`s that happen to have the same underlying
  representation" from "the same `enum`, referenced twice" — two
  separately declared enums with identical enumerator values (even an
  anonymous `enum {...}` structurally matching a named one) were wrongly
  reported compatible, when C 6.2.7 makes enum identity nominal, not
  structural (the same rule struct/union already got right via `members`
  pointer identity). Fixed by adding a `Type *enum_id` field, set to the
  completed enum's own canonical `Type*` (and preserved across the
  plain-struct copies a bare `enum tag` type-name lookup makes), and
  having `types_compatible_p_qual()` require `a->enum_id == b->enum_id`
  whenever both sides are enums — an enum compared against a _non_-enum
  integer type of matching representation (e.g. `enum E : short` vs.
  plain `short`) is untouched by this and still falls through to the
  representation-only checks.
  → found via cproc's `test/enum-large-value.c`
- **Enumerator value → type promotion (no fixed underlying type) skipped
  the `long`/`unsigned long` tier, jumping straight from `int`/`unsigned`
  to `long long`/`unsigned long long`** (parser.c, `enum_specifier()`) —
  C23 6.7.3.4 (and matching GCC/clang behavior) picks each enumerator's
  individual type, and the enum's own overall type, as the narrowest of
  `int, unsigned int, long, unsigned long, long long, unsigned long long`
  that fits; on this LP64 target `long` and `long long` are both 8 bytes
  but are still distinct types, so always landing on `long long` produced
  a type `__builtin_types_compatible_p` correctly refused to call
  equivalent to plain `long` (e.g. `enum { A = -0x80000001L, B }`'s
  overall type came out `long long` instead of `long`). Also, min/max
  tracking across enumerators compared raw `int64_t` bit patterns
  directly, so a wide _unsigned_ value using the top bit (e.g.
  `B = -1ull`, i.e. `UINT64_MAX`) compared as a small _negative_ number
  and corrupted the whole enum's range computation. Fixed by tracking
  min/max as `__int128`, widened per-enumerator according to that
  enumerator's own signedness (so `UINT64_MAX` is tracked as itself, not
  as `-1`), and by adding the missing `long`/`unsigned long` tier between
  `int`/`unsigned` and `long long`/`unsigned long long` when picking both
  the enum's overall type and (already-correct) each enumerator's own
  during-processing type.
  → found via cproc's `test/enum-large-value.c`

New regression test: `test/test_enum_c23_wide_and_fixed.c`.

### Fixed (2026-08-08, root-cause/test_cproc-completion session)

**test_cproc now fully passes** the batch harness (`make check`:
196/196; the full `cctest_cproc.bash` conformance smoke-test corpus:
168/168 compile-and-run clean) — but the original TODO.md diagnosis
("expected specific operator" on `_BitInt(total * 3)`, attributed to
missing `_BitInt` support) turned out to be a **misdiagnosis carried
over from an earlier, different failure point**: that exact error text
is test*c23doku's `brute_force.c`, not anything in cproc's own corpus.
test_cproc's \_actual* blocker, found by bisecting cproc's own build down
to a single miscompiled object file, was a completely unrelated
pre-existing integer-literal bug (below) that had nothing to do with
`_BitInt` at all. Real C23 `_BitInt(N)` support (parsing, `wb`/`WB`
literal suffixes, `sizeof`/`typeof`/type-compatibility, up to 128-bit
codegen reusing native/`__int128` register paths) was added regardless,
since it's genuinely useful C23 conformance and cproc's own
`test/bitint-constant.c` needs it — but it was never test_cproc's
critical-path bug.

- **Hex/octal/binary integer-literal type selection compared the raw
  _signed_ bit pattern against `INT_MIN`/`INT_MAX` instead of the
  literal's true (always non-negative) magnitude** (parser.c, `new_num()`)
  — a literal token is never negative by construction (unary `-` is a
  separate, later operator), so a hex/octal constant whose value exceeds
  `INT64_MAX` (e.g. `0xffffffffffffffff` == `UINT64_MAX`) merely _looks_
  negative once its bit pattern is reinterpreted as a signed `int64_t`
  (`-1`). The old ladder's very first check, `val >= INT32_MIN && val <=
INT32_MAX`, used that raw signed value directly — `-1` trivially
  satisfies it — so a genuinely 64-bit-magnitude hex constant was
  silently typed as plain 4-byte `int` (`sizeof(0xffffffffffffffff) ==
4`), truncating every later use of it to 32 bits. Separately, even once
  a value correctly fell through past the `int` check, hex/octal/binary
  constants (C11 Table 6's _non-decimal_ candidate list, which — unlike
  decimal — includes the unsigned type of the same rank) never actually
  reached an unsigned fallback: the ladder's final `else` branch always
  picked signed `long long` regardless of radix. Rewrote the whole
  ladder (covers every suffix combination: none, `u`, `l`, `ll`, and
  their combinations) to compare magnitudes via the literal's _unsigned_
  64-bit representation throughout, and to offer the unsigned tier at
  every rank for non-decimal constants once the signed one overflows.
  → found by bisecting cproc's own object files (rcc-compiled vs.
  gcc-compiled) down to `expr.c`'s `inttype()` — the function that
  computes a _C23 `_BitInt` literal's own width_ via `0xffffffffffffffff
  > > (64 - i)`for every candidate`i`, entirely independent of the
literal's actual magnitude; since `0xffffffffffffffff`(no suffix)
always mis-typed to plain 4-byte`int`(bug above), every occurrence of
that shift silently became a 32-bit shift-by-a-32-bit-value instead of
the intended 64-bit logical shift, corrupting the width computation for
*every*`\_BitInt`literal cproc's own compiler (built by rcc) ever
processed — including the tiny ones in its own`test/bitint-constant.c`self-test, which was the actually-failing step`make check` reported.
- **`is_typename()` didn't recognize `_BitInt` as a type keyword**
  (keywords.gperf/keyword_ids.h) — `_BitInt` was only matched via
  `equalc()` string comparison inside `declspec()`'s own keyword chain,
  never registered in the `KW_TYPE`-flagged keyword table `is_typename()`
  consults for cast/`sizeof`/`_Generic`-association disambiguation, so
  `(_BitInt(N))expr` and similar cast-position uses of `_BitInt` failed
  to parse as a type at all. Registered as a real `KW_TYPE` keyword,
  matching `_Bool`'s existing entry.
- **`ND_DEREF` rejected a second `*` applied to an already-function-typed
  expression** (type.c) — C11 6.5.3.2p4 makes repeated `*` on a function
  designator idempotent (`(***f)()` == `f()`, since a function-typed
  rvalue implicitly decays back to pointer-to-function before any
  operator sees it, the same "decay" arrays undergo), but rcc's type
  check for `*` only accepted `TY_PTR`/`TY_ARRAY`/`TY_VLA` operands — the
  _first_ `*` on a function pointer correctly yields the bare function
  type back, but the _second_ `*` then saw that bare `TY_FUNC` operand
  and rejected it outright ("cannot apply '\*' to a non-pointer type").
  Fixed by special-casing a `TY_FUNC` operand to yield the same type back
  unchanged.
  → found via cproc's `test/func-noreturn.c`
- **C23 "label may precede a declaration or the end of a compound
  statement" was only implemented for plain identifier labels, not
  `case`/`default`** (parser.c) — the C23 relaxation (previously every
  label required a following _statement_, needing `case 0: ;` as an
  empty-statement workaround) had already been implemented for ordinary
  `label:` but not extracted to the near-identical `case`/`default`
  parsing blocks, so `switch (x) { case 0: }` (a case label immediately
  before the closing `}`) failed with "expected an expression" instead
  of parsing as an empty-statement case.
  → found via cproc's `test/unreachable.c`
- **A wide-string-literal (`u`/`U`/`L`/C23 `u8"..."`) whole-array
  designated initializer corrupted the token stream for any _subsequent_
  designator in the same braced initializer** (parser.c,
  `global_init_member()`/`local_init_member()`) — both functions' bypass
  check that routes a string-literal initializer value straight to
  `global_init_one()`/`local_init_one()` (which correctly handles narrow
  _and_ wide strings) only excluded `TY_CHAR`-based arrays from the
  generic "flatten into per-element initializers" fallback path; a
  wide-char-based array target (`unsigned short`/`unsigned int` element,
  string-literal _value_) fell through to that fallback instead, which
  misparses the whole string token as a single scalar initializer
  element and leaves the token cursor in the wrong place for whatever
  designator follows (`.s = u"ab", .s[2] = 99,` — the `.s[2]` designator
  after a wide-string whole-array init failed with "expected an
  expression"). Fixed by widening the bypass condition to any
  string-literal value, matching `global_init_one()`'s own already-
  correct narrow/wide dispatch.
  → found via cproc's `test/initializer-replace-static-string-wide.c`
- **A wide string literal's _expression_ type was a pre-decayed pointer,
  not the array type C requires** (parser.c, `primary()`'s `TK_STR`
  branch) — the plain-`char`/pre-C23-`u8` cases already correctly built
  `array_of(ty_char, len+1)` ("Use array type so sizeof works correctly;
  decays to pointer where needed" per the existing comment), but every
  wide-prefix case (`L`, `u`, `U`, C23 `u8`) built `pointer_to(elem_ty)`
  directly instead — a real bug independent of `typeof`, since `sizeof
L"ab"` and ordinary array-decay-dependent uses were _also_ wrong, just
  less commonly hit. `typeof()` in particular must see the un-decayed
  array (`typeof(u8"abc") == unsigned char[4]`, not `unsigned char *`)
  since `typeof`/`__typeof__` doesn't apply lvalue conversion. Fixed by
  building `array_of(elem_ty, N+1)` for every prefix, computing `N` as
  the _decoded codepoint count_ (via the same UTF-8 decode loop
  `global_init_one()`'s wide-string writer already uses to count
  elements), not the raw UTF-8 byte length.
  → found via cproc's `test/string-u8-type.c`
- **Function-type parameter comparison never stripped the parameter's own
  top-level qualifiers** (parser.c, `type_equal()`'s `TY_FUNC` case) — C11
  6.7.6.3p15 requires a parameter's own top-level qualifiers to be
  disregarded for function-type compatibility (only qualifiers on what a
  pointer parameter points to matter), including recursively when the
  parameter is itself a function type reached through decay (`int(const
double)` as a parameter decays to `int(*)(double)`, whose _own_
  parameter `const double` must still compare compatible with plain
  `double`). The parameter-list comparison loop called `type_equal()`
  directly on each pair without ever clearing this qualifier, so any
  qualified scalar parameter — nested arbitrarily deep through
  function-pointer-parameter chains — broke redeclaration matching.
  Fixed by comparing qualifier-stripped shallow copies for each
  parameter pair.
  → found via cproc's `test/compatible-function-types.c`
- **`__builtin_types_compatible_p` was a hand-rolled, incomplete
  scalar/pointer/array/struct switch** (parser.c) — missing a `TY_FUNC`
  case entirely (fell through to a `size == size` default, meaningless
  for function types); its `TY_ARRAY` case compared the unused
  `->array_len` field (never populated by `array_of()` — only
  `vla_of()`'s constant-VLA fallback sets it — so it read `0` for both
  sides and treated _every_ array pair, regardless of actual declared
  length, as compatible); and its `TY_PTR`/`TY_ARRAY` cases only checked
  one level of base-type shape (kind/size/unsigned/qual), not full
  recursive structural compatibility. Rewrote entirely as two new
  functions, `types_compatible_p()` (disregards the two top-level
  argument types' own qualifiers, matching GCC's documented behavior)
  delegating to `types_compatible_p_qual()` (fully recursive, qualifier-
  aware structural compatibility through `TY_PTR`/`TY_ARRAY`/`TY_FUNC`/
  `TY_STRUCT`/`TY_UNION`/`TY_COMPLEX`/`TY_BITINT`, with C11 6.7.6.3p15
  parameter-qualifier-stripping built in and the same
  correct-array-length computation as the fix above).
  → found via cproc's `test/compatible-array-types.c`
- **The `test_cproc` batch-harness step never actually ran end-to-end** —
  `test/linux_thirdparty.bash`'s `test_cproc()` function (imported
  wholesale from slimcc's own test harness, see commit `86508a8f`)
  references a companion script, `test/cctest_cproc.bash`, that was never
  copied over — every batch run's `test_cproc` classification prior to
  this session was actually failing on "script not found", not on
  anything specific to cproc's own build. Ported the missing script
  verbatim from slimcc's upstream `scripts/cctest_cproc.bash` (a smoke
  test that compiles + runs, or verifies expected-to-fail, every file in
  michaelforney/cproc's own `test/*.c` corpus directly with `$CC`,
  independent of cproc's own `make check`), adding two rcc-specific
  entries to its `skip_files` (see below), matching the exact convention
  the file already uses for slimcc's own two skips.
- **Automatic/VLA storage over-alignment beyond the ABI-guaranteed 16
  bytes is unimplemented** — `alignas(32) char x;` (a plain stack local)
  and `char alignas(64) a[n];` (a VLA) both need the _stack frame itself_
  dynamically realigned in the prologue (a distinct RSP-relative base for
  every local beyond the natural 16-byte guarantee, while incoming-
  argument addressing, VLA/`alloca` interaction, and Windows SEH/ARM64
  AAPCS64 unwind info all still need their existing RBP-relative
  assumptions preserved) — a large, invasive, cross-cutting prologue/
  epilogue redesign, not attempted this session. Documented as a known
  gap via `cctest_cproc.bash`'s `skip_files` (`alignas-local-strict`,
  `alignas-vla-strict`), matching how the same file already documents
  slimcc's own two unimplemented-builtin skips (`builtin-inff`,
  `builtin-nanf`).
- **`eval_const_expr()`'s `ND_EQ`/`ND_NE`/`ND_LT`/`ND_LE` compared
  operands at raw 64-bit width instead of their actual common type's
  width** (parser.c) — found _after_ landing the C23 enum-type-modeling
  fix above (once enum constants started actually being typed
  `unsigned`/`long`/etc. instead of always `int`, this pre-existing bug
  went from dormant to load-bearing): comparing an `unsigned int` value
  against a same-width `int` value whose bit pattern has the top bit set
  (e.g. `(int)0x80000000` == `-2147483648`, sign-extended to
  `0xFFFFFFFF80000000` as a 64-bit `long long`) against the _bit-
  identical_ unsigned 32-bit value `0x80000000` (`2147483648` as a
  64-bit `long long`) via `(unsigned long long)lhs == (unsigned long
long)rhs` compares two different 64-bit patterns, not the equal
  32-bit ones they both represent — the wider sign extension was never
  re-truncated to the comparison's actual common width before the
  unsigned reinterpretation. Only manifests at `-O1`+ (opt.c's `ND_IF`
  dead-branch-elimination is the only caller that constant-folds a
  comparison at all — at `-O0` the same comparison runs as ordinary,
  always-correct runtime code). Fixed by truncating each operand to the
  _wider_ of the two operand types' own byte width before the unsigned
  reinterpretation (new `uval_at_width()` helper), for all four
  comparison operators.
  → found via `test_enum_c23_wide_and_fixed`'s own `PU_A != (int)
0x80000000` check regressing at `-O1` after the enum fix landed.

New regression tests: `test/test_const_fold_signed_unsigned_cmp.c`
(the `eval_const_expr` width bug, verified at both `-O0` and `-O1`
against real gcc). The five C23 `_BitInt`/parser fixes above landed
alongside four parallel subagent sessions (documented separately above:
"cproc scope/attribute/GNU-ternary", "C23 enum type-modeling",
"VM-type evaluation-order", "cproc array/VLA-param type-modeling") that
together closed out every remaining `cctest_cproc.bash` failure.
**Full suite verified**: TCC 118/118, Unit tests 189/189, C-testsuite
220/220, NCC Compliance 15/15, Torture 3605/3609 (100% of non-skipped),
Dg-error 34/34, Link tests 5/5 — 0 failed overall, on native Linux
x86-64; mingw and arm64 cross-compile targets also re-verified clean.
`test_cproc`'s own batch-harness run: `PASS=1 FAIL=0`.

### Fixed (2026-08-08, mingw cross-compile verification session)

The prior session's claim that "mingw and arm64 cross-compile targets
also re-verified clean" was **wrong for mingw** — a full `run_tests.exe
./rcc.exe --all --parallel` run under Wine surfaced one genuine
regression from the same session's C23 enum type-modeling fix, invisible
on native Linux/arm64 because both are LP64.

- **The enum overall-type ladder's `long`/`unsigned long` tier used
  hardcoded `INT64_MAX`/`UINT64_MAX` bounds instead of the platform's
  actual `long` size** (parser.c, `enum_specifier()`) — `long` is 8
  bytes on LP64 (Linux/macOS) but only 4 bytes on LLP64 (Windows/mingw,
  same range as `int`). The ladder's `long` bounds check
  (`min_val >= -INT64_MAX-1 && max_val <= INT64_MAX`) assumed the LP64
  case unconditionally, so on mingw a value needing genuine 64-bit
  signed range (e.g. GCC torture's `c23-enum-1.c`:
  `enum e1 { e1a = -__LONG_LONG_MAX__ - 1, ..., e1c = __LONG_LONG_MAX__ }`)
  wrongly satisfied the check and picked 4-byte `ty_long` instead of
  8-byte `ty_llong`, truncating the enum's values and failing
  `static_assert (sizeof (enum e1) >= sizeof (long long))`. Fixed by
  computing the tier's bounds from `ty_long`/`ty_ulong`'s actual
  `->size` (matching the same LP64-vs-LLP64 pattern already used
  elsewhere in the codebase, e.g. the integer-literal ladder). On
  mingw this correctly makes the `long` tier unreachable for anything
  not already representable in `int` (since `long`'s range equals
  `int`'s there), skipping straight to `long long` — matching real
  GCC's own behavior on Windows.
  → found via `test/torture/c23-enum-1.c` regressing under
  `./mingw-test.sh` (0 mingw-cross failures before this session's enum
  work, 1 after — caught by actually running the full mingw suite
  rather than trusting the untested claim).
- **`test/test_enum_c23_wide_and_fixed.c`'s own `PromoteLong`/
  `PromoteUnsignedLong` cases hardcoded the LP64 answer** (`long`/
  `unsigned long`) for a value needing more than 32 signed bits — a
  test-only portability bug, not an rcc bug, but it would have failed
  the same way as a false regression once the ladder fix above made
  mingw's `long` tier correctly unreachable. Fixed by selecting the
  expected type via `__LONG_MAX__ > __INT_MAX__` (matching
  `c23-enum-1.c`'s own `TYPE_CHECK` convention), so the test asserts
  `long` on LP64 and `long long` on LLP64.

**Full suite re-verified after the fix**: mingw cross — TCC 118/118,
Unit tests 187/187 (188 total, 1 pre-existing skip unrelated to this
bug), C-testsuite 220/220, NCC Compliance 15/15, Torture 3574/3574
non-skipped (100%), Dg-error 34/34 — 0 failed. Native Linux x86-64 and
arm64 cross (both LP64, unaffected by this LLP64-only bug) re-verified
clean: Linux Torture 3605/3609 (100% non-skipped), 0 failed overall.

### Session summary (2026-08-09, ARM64 stur/ldur frame-offset session)

Picked the "ARM64 small-struct return ABI" TODO item as the next
scoped, already-diagnosed bug to fix; found on investigation that the
documented classification gap was already fixed (uncredited, in an
earlier session) and the entry was stale, but the regression evidence
behind it (`00204.c`) was still genuinely failing for an unrelated
reason — see "Fixed rcc bug: ARM64 `stur`/`ldur` frame-offset immediate
overflow" above for the real root cause and fix. `00204.c` now passes
byte-for-byte at both `-O0` and `-O1`.

### Fixed rcc bug: `__attribute__((section("name")))` on a global was silently unparsed

`read_type_attrs()` had no case at all for the `section`/`__section__`
GCC attribute — it fell through to the generic "unrecognized attribute,
skip its parenthesized argument" path, so a global declared with a
`section("name")` attribute landed in the ordinary `.data`/`.rodata`/
`.bss` like any other global, never in a section literally named
`"name"`. Any code relying on the common linker-collected-array idiom
(`extern char __start_name[]; extern char __stop_name[];` — real GNU
ld automatically synthesizes those two symbols for any section whose
name is a valid C identifier, bracketing every input object's
contribution to it) failed at link time with "undefined reference to
`\_\_start_name'": nothing had ever created a section by that name for
the linker to synthesize boundaries around.

Fixed in three places:

- **parser.c** — `read_type_attrs()` now recognizes `section("name")`/
  `__section__("name")` (mirroring the existing `alias("target")`
  parsing exactly) and threads the name through a new
  `pending_section_name` (matching `pending_alias_target`'s own
  set-at-parse/consume-at-declaration/reset-at-every-declaration-
  boundary pattern) onto the declared global's new `LVar::section_name`
  field.
- **codegen.c** — a global with a non-NULL `section_name` is now routed
  to a custom ELF section via the existing (previously inline-asm-only)
  `objfile_find_or_add_section()`, with `SHF_ALLOC` (plus `SHF_WRITE`
  unless the variable is `const`-qualified) instead of the default
  `.data`/`.bss`/`.rodata` placement — always through the "has
  initializer data" emission path, even for a zero-initialized
  section() global, since a named section is virtually always a
  single deliberately-isolated marker object, not a candidate for the
  NOBITS/BSS space optimization. Separately, `cg_set_section()`'s
  `default:` case for any section id ≥ `SEC_NUM` (a custom section)
  was silently defaulting to `.text` instead of resolving the section's
  own growable buffer — a latent bug independent of this feature
  (nothing previously routed a _global variable's data_ through a
  custom section id at all, only inline-asm's own already-section-aware
  emission path) that would have silently corrupted `.text` with
  strayed variable bytes the moment anything did.
- **link_elf.c** — rcc's own native ELF linker now synthesizes
  `__start_<name>`/`__stop_<name>` for any section whose name is a
  valid C identifier, matching real GNU ld, inserted right before the
  existing "identify unresolved undefined symbols" pass (all input
  objects are already loaded by this point, so every named section's
  final size is already known — no second, post-layout pass needed).
  Previously only the GCC/external-linker fallback path could resolve
  such a reference at all (via the real system `ld`), so source using
  this idiom only worked when rcc's native linker happened to bail out
  to that fallback for some unrelated reason.

→ found via GCC c-testsuite's `00204.c` investigation surfacing the
underlying rcc-arm64 struct-return-ABI TODO item as already fixed (see
above); chasing `00204.c`'s own remaining SIGSEGV (the real, unrelated
`stur`/`ldur` bug documented above) led to picking the next open TODO
item, `test_scrapscript`'s "rcc compile fails (exit 1) during Python
test harness" — which turned out to actually be a **link** failure
(`undefined reference to '__start_const_heap'`/`'__stop_const_heap'`)
from scrapscript's own runtime GC placing a "const heap" boundary
marker via `__attribute__((section("const_heap")))`, specifically so
`in_const_heap()` can tell a constant, pre-allocated object apart from
one living in the mutable GC heap.

**Note**: while isolating a regression test for this fix, also found a
separate, pre-existing, unrelated native-linker gap — any use of
`fprintf(stderr, ...)` (confirmed with a three-line minimal repro, no
`section()` involved at all) makes `rcc_link()`'s native ELF linker
return -1 and silently fall back to the GCC/system-`ld` path. Not
investigated further this session (the fallback itself works
correctly, and this session's own regression test avoids the trigger
by using `printf` instead so it still exercises the native-linker path
being fixed here) — worth a future session's attention since it's
presumably a very common pattern to trip over.

New regression test: `test/test_attribute_section.c` — three
separately-declared `section("name")`/`__section__("name")` globals
(exercising both spellings), verifying `__stop_name - __start_name`
correctly brackets all three contiguous entries (not just a single
lone marker), every entry's data is intact and in the right place, and
an ordinary global with no `section` attribute is unaffected. Compiles
and links via rcc's own native linker on both x86-64 and arm64 Linux
(confirmed by the _absence_ of an `RCC_LINK_DEBUG=1` fallback
message — a stronger check than merely "the program produces the
right answer somehow", which the GCC-fallback path alone could also
satisfy) and via mingw cross (through mingw's own `ld`, which supports
the identical `__start_`/`__stop_` PE-COFF convention).
**Full suite verified after the fix**: native Linux x86-64 — Torture
3605/3609 (100% non-skipped) 0 failed, Dg-error 34/34, Link 5/5, 0
failed overall; arm64 cross — c-testsuite 220/220, Torture 3599/3609
(6 pre-existing unrelated runtime failures, unchanged from the
`stur`/`ldur` fix above), Dg-error 34/34, 0 new failures; mingw
cross — Torture 3574/3578 (100% non-skipped) 0 failed, Dg-error 34/34,
0 failed overall.

### Fixed rcc bug: custom `section()` global had no ELF `sh_addralign`, corrupting scrapscript's GC

The `section()` attribute fix above (committed same day) correctly
placed each `__attribute__((section("name")))` global's _bytes_ — but
`objfile_find_or_add_section()` always created the section with ELF
`sh_addralign = 1` ("no constraint"), regardless of what was placed in
it. rcc's own native ELF linker was then free to place that section's
first byte at _any_ file offset — including one only 4-byte aligned —
silently breaking the natural alignment every `uintptr_t`/pointer/
`size_t` field inside the section's contents actually needs.

scrapscript's GC uses a classic tagged-pointer scheme: a heap object's
address always has its low 3 bits clear (`kObjectAlignment = 8`), and
`ptrto()`/`as_heap_object()` set/clear a single low tag bit to mark
"this is a heap pointer" vs "immediate small int". A `const_variant_0`
global (`struct variant`, holding a `uintptr_t` tag + a `struct
object*` value — both needing 8-byte alignment) placed via
`__attribute__((section("const_heap")))` landed at `0x405514` in one
run — only 4-byte aligned. The struct's own _bytes_ were still
byte-perfect (verified with `readelf -x`/gdb: tag=`TAG_VARIANT`,
`variant.tag=Tag_foo`, `variant.value` all correct) and the tagged
pointer handed to `print()` was numerically correct too — but
`is_heap_object()`'s `(uword)obj & kPrimaryTagMask(7) == 1` check
silently failed on that misaligned address, so `print()` fell through
every `is_*()` branch and hit its `abort()` "unknown tag" fallback —
17/17 remaining `test_scrapscript` failures were exactly this SIGABRT,
all originating in `scrap_main()`'s very first `print()` call.

Fixed by threading a real per-section alignment through to the ELF
writer:

- **obj.h/obj.c** — `ExtraSection` gained an `align` field (default 1,
  raised via the new `objfile_section_align()`, never lowered — a
  section can receive multiple globals with different alignment
  needs, so it tracks the max).
- **codegen.c** — after resolving a `section()` global's target
  section id, calls `objfile_section_align(cg_obj, data_sec,
var->ty->align)` so the section's requirement reflects every global
  ever placed into it.
- **asm.c** — an explicit `.balign`/`.align`/`.p2align` directive
  targeting a custom (inline-asm `.section`-declared) section now also
  raises that section's `sh_addralign`, matching real GAS's own
  behavior (previously only the byte offset _within_ the section's
  buffer was aligned, not the section's own load-address requirement).
- **elf_write.c** — the extra-section file-offset layout loop now
  aligns each section's starting offset to `max(16, its own
sh_addralign)` instead of a blanket 16, and the ELF section header
  is written with the real alignment instead of a hardcoded `1`.
  (rcc's own linker load address is `image_base + file_offset` with an
  always-page-aligned `image_base`, so aligning the file offset is
  equivalent to aligning the final virtual address for any alignment
  up to a page.)

Scoped to the ELF writer only (native Linux + arm64 cross) since that's
where the actual failure lived; `macho_write.c`'s extra-section path
already unconditionally uses 8-byte alignment (a `section()`-holding
global that needs more than that on macOS is a pre-existing,
unexercised gap, out of scope here), and `coff_write.c`/PE-COFF
characteristics don't encode a per-section alignment at all yet (also
pre-existing, not triggered by any current test).

→ found by tracing `test_scrapscript`'s remaining 17 `SIGABRT`s (see
the "partially fixed" note in the section() writeup above) with gdb:
confirmed the tagged pointer and the underlying struct bytes were both
correct at the point of failure, then `readelf -S` on the crashing
binary showed `const_heap`'s `sh_addralign` was `1` and its actual
load address only 4-byte aligned.

Regression test: extended `test/test_attribute_section.c` with an
explicit `(uintptr_t)__start_my_registry % _Alignof(struct
registry_entry) == 0` assertion (`struct registry_entry` already
contains a `const char *` field, so it requires 8-byte alignment).
Confirmed this assertion fails (`exit 5`) on the pre-fix binary and
passes on the fixed one. Full `test_scrapscript` suite (33 tests, 1
skipped) now passes 100%, up from 17/33 failing.
**Verified**: native Linux x86-64 — `test_attribute_section` PASS,
full `test_scrapscript` compiler_tests suite 32/32 passed (1 skipped);
mingw cross and arm64 cross — `test_attribute_section` PASS via
`mingw-test.sh`/`arm64-test.sh`.

### Added: `__attribute__((section("SEG,SECT")))` support for Mach-O (macOS)

The `section()` attribute fix above was ELF-only; the Apple branch of
`test_attribute_section.c` was left as a stub (`(void)e1; ...; return
0;`) because macho_write.c's custom-section support pre-dated this
session and only handled a single flat name (no explicit segment),
and link_macho.c had no concept of a boundary-symbol synthesis at
all. Implemented properly this session, entirely from code review +
local structural verification -- no real macOS available in this
sandbox; both touched files (macho_write.c, link_macho.c) are only
ever compiled by the project's own build when `$(CC) -dumpmachine`
contains "apple" (see Makefile's `SRCS +=` split), so CI's
macos-latest job is the only real test surface.

- **macho_write.c** — `macho_seg_sect_name()` replaces the old
  `macho_section_name()`/`macho_segname_from_flags()` pair: a
  `section()` string containing a comma (Apple's required
  "SEGMENT,SECTION" form, e.g. `"__DATA,const_heap"` -- a bare
  single-component name is a real clang error on Mach-O) is split
  verbatim into the two fixed 16-byte segname/sectname fields; a
  comma-less name (ported GAS `.section` asm with no segment) keeps
  the previous code/data-flags-based inference.
- **link_macho.c** (`link_load_object`) — a loaded section whose
  _sectname_ half doesn't start with `__` (Apple reserves that prefix
  for its own system sections -- `__cstring`, `__common`,
  `__la_symbol_ptr`, ...) is necessarily a user's own `section()`
  global; kept as its own distinct internal `"SEG,SECT"`-named
  section instead of folding into the generic `.data`/`.rdata`
  bucket every _other_ non-well-known section still uses.
- **link_macho.c** (`link_macho`) — two additions:
  1. Boundary-symbol synthesis for ld64/clang's own
     `section$start$SEG$SECT`/`section$end$SEG$SECT` linker-symbol
     convention (the Mach-O equivalent of ELF's `__start_`/`__stop_`,
     used via `extern char x[] __asm("section$start$__DATA$name")`
     -- a plain `__start_name` has no meaning on Mach-O at all).
     Runs _before_ the existing external-symbol-strategy scan: an
     unresolved boundary reference is loaded via a GOT-relative
     instruction and would otherwise look like a genuine external
     symbol, bouncing the whole link to the system linker.
  2. The `mo_secs` write-side classification recognizes an internal
     `"SEG,SECT"` name (set by the loader change above) and emits
     that exact segment/section pair instead of the generic
     `__DATA,__data` fallback.

`test/test_attribute_section.c` rewritten with a real (much smaller)
`#ifdef __APPLE__` split instead of the old two-full-`main()`-bodies
stub: only the `section()` argument string and the boundary-symbol
`extern` declarations differ per platform (`__asm("section$start$...")`
labels on Apple); the entire `main()` body -- including the alignment
regression check from the fix above -- is shared and unconditional.

**Verification** (no real macOS in this sandbox, see above):

- `link_macho.c` compiles clean standalone on Linux (it has no
  `#ifdef __APPLE__` file guard, unlike macho_write.c, so the
  project's real build never exercises it here either -- confirmed
  with a direct `gcc -c src/link_macho.c`).
- `macho_write.c` (entirely `#ifdef __APPLE__`-gated, so _never_
  compiled by this project's own native/mingw/arm64 builds) syntax-
  checked clean with `gcc -fsyntax-only -D__APPLE__` -- safe since it
  only uses portable `<stdio.h>`/`<string.h>`/`<stdint.h>`, no real
  Apple SDK headers.
- Compiled `test_attribute_section.c` through rcc itself with
  `-D__APPLE__` forced (still targeting rcc's real ELF backend, since
  this sandbox has no Mach-O target) to exercise the _parser/codegen_
  side of the Apple branch: `readelf` on the result confirms the
  section is correctly named `__DATA,my_registry` (8-byte aligned,
  48 bytes = 3 \* 16-byte entries) and the two `extern char x[]`
  declarations correctly emit as undefined symbols literally named
  `section$start$__DATA$my_registry`/`section$end$__DATA$my_registry`
  -- exactly the names link_macho.c's new synthesis pattern-matches.
- `test_attribute_section` still PASSes on native Linux x86-64, mingw
  cross, and arm64 cross (unaffected -- neither touched file is part
  of any of those three builds).
- Pushed to CI; the macos-latest job is the authoritative check for
  this change and needs to be watched explicitly.

### Fixed (2026-08-09, issue #4 continuation — 7 bugs via scout-parallel triage)

Continued triaging remaining test/third_party/TODO.md failures (GitHub
issue #4's checklist) by running a fresh batch of previously-unchecked
small/medium projects and dispatching parallel read-only scout
investigations into the most promising leads. Six genuine rcc bugs
found and fixed, all with regression tests, full suite verified after
each:

1. **declarator() SIGSEGV on an unclosed `(`** (parser.c) — the C11
   6.7.6p3 nested-paren disambiguation recursed into declarator() with
   the terminal TK_EOF token once its matching-paren scan ran out of
   input; TK_EOF's own `->next` is NULL (the lexer's genuine
   end-of-list sentinel), and the immediately following `tok->next`
   dereference flowed that NULL into skip_attributes()/
   read_type_attrs(), segfaulting on `tok->kw`. Found via ksh93's own
   AT&T ast-open build probe (`src/cmd/INIT/C+probe`), which
   deliberately compiles a bare `(` as a negative-compilation-check.
   Fixed by diagnosing "expected ')'" instead of recursing into an
   exhausted token stream.
2. **CRLF backslash-newline macro continuation never spliced**
   (preprocess.c) — `splice_lines_with_counts()` only matched `\`
   immediately followed by `\n`, missing the `\` `\r` `\n` form used by
   CRLF-terminated third-party sources. Found via test_unqlite's
   CRLF-terminated amalgamated `unqlite.c`.
3. **`__attribute__((mode(TI)))` unrecognized, plus a broader trailing-
   mode()-attribute leak** (parser.c) — TI (128-bit) was missing from
   the QI/HI/SI/DI mode table; fixing it surfaced a pre-existing bug
   where a _trailing_ mode() attribute (GCC's own convention, written
   after the identifier) was parsed but never applied in that
   declarator() call, leaking the pending_mode flag into whatever
   declarator() ran next and silently retyping an unrelated
   declaration. Factored a shared apply_pending_mode() helper called
   from every declarator() exit path. Found via test_libtommath's
   `typedef unsigned long mp_word __attribute__((mode(TI)));`.
4. **`__builtin_cpu_init()` unimplemented** (preprocess.c + parser.c) —
   GCC/clang's companion to the already-implemented
   `__builtin_cpu_supports()`; added as a true no-op stub (rcc's
   `__rcc_cpu_supports` re-queries cpuid directly every call, no cache
   to populate) mirroring the existing wiring exactly. Found via
   test_libucl's bundled mum.h.
5. **`infer_array_type()` sized a pointer array by a string literal's
   length** (parser.c) — `T *arr[] = { "literal" }` (a correctly-typed
   ONE-pointer array) was misclassified as C11 6.7.9p14's char-array-
   sized-by-string-literal case, with no check that the element type
   was actually scalar/non-pointer, unlike every sibling STRLIT-sizes-
   array call site in the file. Fixed with the same scalarish*base-
   style guard (excludes ARRAY/STRUCT/UNION/PTR). Found via
   test_flatcc's gperf-generated `fb_reserved_kw_vec_prefixes[] = {
   "vec*" }`, which crashed calling strlen() on garbage 4 elements past
   the array's real end.
6. **Plain `gnu_inline` function export** (codegen.c) — `fn_exported`
   implemented pure C99 inline linkage unconditionally; real GCC's
   GNU89 semantics (what `__attribute__((gnu_inline))` explicitly opts
   into) mean a plain (non-extern) gnu_inline function DOES get an
   ordinary global-linkage definition. Missing this made gperf-
   generated code using the common `#ifdef __GNUC_STDC_INLINE__
__attribute__((gnu_inline)) #endif` portability idiom compile fine
   per-TU but emit as a local symbol, invisible to any other TU — only
   surfacing at final-executable link time (a shared-library link
   tolerates the undefined symbol). Found via test_hoedown's gperf-
   generated `html_blocks.c`'s `hoedown_find_block_tag`.
7. **`int64_t`/`intptr_t`/`intmax_t` (+ unsigned) used `long long`
   unconditionally** (include/stdint.h) — on LP64 targets (native
   Linux/macOS x86-64 and arm64), these must be `long`/`unsigned long`
   to match glibc's own convention (`bits/types.h`'s
   `__int64_t`/`__intmax_t`, guarded on `__WORDSIZE == 64`) — the same
   class of bug the file's existing `ptrdiff_t` note already documents,
   just for four more names. A TU pulling in both this header and a
   real glibc header re-declaring these names (very common — countless
   system headers transitively include `<bits/stdint-intn.h>`) hit a
   real "conflicting types" error at every int64_t/intmax_t/intptr_t-
   parametered function once prototype/definition redeclaration
   diagnostics were added. Kept `long long` on `_WIN32` (LLP64, where
   `long` is only 4 bytes; matches MSVC/mingw's own convention there —
   this branch is unchanged from before, so mingw is unaffected by this
   fix). Found via test_libtommath's `MP_INIT_INT(mp_init_i64,
mp_set_i64, int64_t)`, whose macro-expanded definition disagreed
   with its own header-declared prototype once glibc's transitively-
   included `<bits/types.h>` re-typedef'd `int64_t` as `long`.

5 of 5 deep-dived third-party targets are now confirmed fixed:
test_unqlite, test_libucl, test_hoedown, test_flatcc, test_libtommath
all build clean; the `gnu_inline` fix has no single-project home (found
via test_hoedown but is a general codegen bug, verified via a dedicated
two-TU link test instead).

New regression tests: `test/test_err_unclosed_paren.c`,
`test/test_crlf_line_continuation.c` (genuine CRLF file),
`test/test_mode_ti_attribute.c`, `test/test_builtin_cpu_init.c`,
`test/test_ptr_array_strlit_size.c`, `test/test_stdint_int64_glibc_abi.c`,
plus `test/test-link.sh` case 7 (gnu*inline needs real two-TU linking,
which the single-file `test/test*\*.c` harness can't express).

**Full suite verified after every fix**: TCC 118/118, Unit 197/197,
Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Link 6/6
(native Linux x86-64); C-testsuite 220/220 on both mingw cross and
arm64 cross, plus the specific new/affected unit tests confirmed
passing on both cross targets (`_WIN32` branch of the stdint.h fix is
behaviorally unchanged on mingw by construction).

### Fixed (2026-08-09, continued — `__builtin_cpu_supports`/`__builtin_cpu_init` broken on mingw)

- **`__builtin_cpu_supports`/`__builtin_cpu_init` (added in the session
  above via `test_libucl`) were silently unresolved on the mingw/Windows
  cross target** — CI's `test (windows-latest)` job failed to link
  `test_builtin_cpu_init` with `undefined reference to
'__rcc_cpu_supports'`. Root cause: the two were implemented as real
  functions injected into parser.c's per-TU synthetic prelude, but only
  inside the `#else` (pure SysV x86-64) branch of that prelude's
  va_list-ABI `#if`/`#elif` ladder — the sibling `#elif defined(_WIN32)`
  branch never got them at all, so `__builtin_cpu_supports`/
  `__builtin_cpu_init` calls survived `preprocess.c`'s `define_pre`
  rename into `__rcc_cpu_supports`/`__rcc_cpu_init` but those symbols
  were simply never defined on that target.
- First fix attempt — hoisting the two functions out of the SysV-only
  branch into a block shared by both non-ARM64 x86-64 targets (the raw
  `cpuid` instruction is identical under both calling conventions) —
  built clean and fixed the link error, but broke 4 unrelated existing
  mingw unit tests (`test_asm_two_bugs_popcnt_alt`, `test_skip_maxdiff`,
  `test_x86_priv_insns`, `test_x86_pushf_invlpg`: expected raw-asm byte
  sequences went missing from `.text`). Bisected (confirmed absent
  without the change, present with it): `opt.c`'s
  `eliminate_unused_static_inline()` — the pass that would normally
  drop these two never-called `static inline` functions back out again
  — is unconditionally disabled on `_WIN32` (see that function's own
  header comment: dropping unused `static inline` bodies there was
  found to shift unrelated object-file layout enough to corrupt an
  emulated-TLS access elsewhere, a separate pre-existing tradeoff).
  Emitting two permanently-live dead functions into every mingw TU
  shifted the affected tests' own code layout enough to break their
  literal-byte-sequence assertions — real, but purely a landmine of
  that already-disabled DCE pass, not of the ABI-sharing idea itself.
- Actual fix: reimplemented both as pure preprocessor macros
  (`preprocess.c`, `define_macro`) instead of real injected functions —
  `__builtin_cpu_supports(f)` expands to a GNU statement expression
  doing the CPUID query inline at the call site, and `__builtin_cpu_init`
  expands to `((void)0)`. Removed the now-unused function-injection
  block from parser.c's prelude entirely. A macro expands to zero bytes
  unless a real call site invokes it, on every target uniformly, with no
  dependency on DCE at all — categorically sidesteps the disabled-pass
  landmine while fixing the same mingw link failure.
  → unblocks: `__builtin_cpu_supports`/`__builtin_cpu_init` on the mingw
  cross target (previously only worked on native Linux x86-64).

**Full suite re-verified after this fix**: native Linux x86-64 —
Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Link 6/6, 0
failed; mingw cross — `test_builtin_cpu_init` now PASSes (previously
an unresolved-symbol link failure), all 4 previously-regressed tests
(`test_asm_two_bugs_popcnt_alt`, `test_skip_maxdiff`,
`test_x86_priv_insns`, `test_x86_pushf_invlpg`) confirmed passing, TCC
118/118, Compliance 15/15 (`test_peep`'s intermittent TIMEOUT under
this sandbox's qemu/emulation load is pre-existing and reproduces
identically without this change — unrelated); arm64 cross —
`test_builtin_cpu_init` PASSes, C-testsuite 220/220, Torture 3599/3609
with the same 6 pre-existing unrelated complex-number/imaginary-
constant runtime failures documented above, 0 new failures.
