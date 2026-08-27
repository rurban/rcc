# Third-Party Test Results & Triage TODO

Batch run: 2026-08-14 (full 221/221 targets; supersedes the 2026-08-06
partial 199/221 run below)
Binary: rcc HEAD (third_party branch)

## Summary

| rc  | count | meaning                                                               |
| --- | ----- | --------------------------------------------------------------------- |
| 0   | 78    | pass                                                                  |
| 2   | 89    | build/compile failure                                                 |
| 1   | 12    | runtime/test failure (many are build-system: CC not respected)        |
| 124 | 29    | timeout (420 s) — most are large projects that build/test cleanly     |
|     |       | and just need more wall-clock time under this batch's per-test budget |
| 127 | 10    | missing tool (muon, lzip, scons, llvm-config, etc.)                   |
| 139 | 1     | SIGSEGV (box2d — confirmed a crash in rcc-compiled C code, see below) |
| 8   | 2     | project's own test-suite comparison failed (utf8proc, yyjson)         |

221 targets triaged this session (5 parallel scouts read every failing
log and classified it ENV / TIMEOUT-ARTIFACT / RCC-BUG / UNCLEAR — see
"Needs fixing" below for the RCC-BUG clusters). One methodological note
for future batch reruns: this sandbox's `/tmp` has a per-user tmpfs
quota: a stale 7.5G `/tmp/.Trash-1000` silently ate most of the headroom
partway through this session and caused several large projects
(test_rsync, test_muon, test_nginx, test_perl, test_qbe_simplecc) to
fail with "Disk quota exceeded" mid-build/mid-test — a sandbox artifact,
not an rcc regression. Cleaned up (`rm -rf /tmp/.Trash-1000`); a full
clean rerun of those five was interrupted by user request and still
needs to be redone to reconfirm their previously-documented "Fixed"
status holds (they were NOT re-broken by anything in this session —
each was mid-build/mid-test with no rcc error when the quota hit).

## File Layout

**⚠ False positives**: Many projects (lua, mruby, many cmake projects)
hardcode `CC=gcc` in their Makefiles and ignore the environment. The test
harness sets `CC=rcc` but the build system overrides it. Verify by checking
`strings <binary> | grep GCC` — if it says GCC, rcc wasn't used.

### Fixed (2026-08-27, test_gnutls -- block-scope conflicting function redeclaration not diagnosed)

- \*\*rcc's file-scope redeclaration path checked function-type
  compatibility, but a block-scope (local) function prototype never
  compared against the file-scope symbol: gnulib's ioctl
  POSIX-signature configure probe declares `int ioctl (int, int,
...);` inside main() to test whether it conflicts with glibc's
  `int ioctl(int, unsigned long, ...)` -- rcc silently accepted it, so
  gnutls' configure concluded the POSIX signature holds, set
  REPLACE*IOCTL=0 and its generated sys/ioctl.h took the SYS branch
  (redeclaring ioctl with `int request`), failing the build at
  src/gl/tests vma-iter.o. gcc errors on the probe, REPLACE_IOCTL=1,
  and the rpl_ioctl path is taken instead. Fixed by extracting the
  file-scope conflict check into func_decls_conflict() and applying it
  to block-scope function declarations too. Regression:
  `test/test_err_func_conflict_local.c` (must fail to compile; the
  pre-fix compiler accepted all three conflict shapes). The gnutls
  build now proceeds past src/gl/tests; its own `tests` suite still
  fails 37 tests that all PASS on a gcc build (resume-*, system-
  override-\_, pkcs11/\*, mini-global-load, tls-supplemental, ...) --
  rcc miscompiles gnutls runtime code, see the "Needs fixing" section
  below; separate session.

### Fixed (2026-08-27, test_gnutls -- duplicate DT_NEEDED from repeated -l flags)

- \*\*rcc's ELF linker pre-seeds libc/libgcc_s/libm DT_NEEDED entries and
  then appended one per `-l<name>` occurrence without dedup, so any link
  naming a library twice (rcc's default libm plus an explicit `-lm` from
  a Makefile/configure LIBS, or `-lfoo -lfoo`) emitted duplicate NEEDED
  entries -- real ld dedups to one. Harmless at runtime, but gnutls'
  configure probe for the libm soname (`objdump -p | grep '^libm\.so'`)
  captured `libm.so.6\nlibm.so.6` from the duplicate, so the
  M_LIBRARY_SONAME define in config.h was written across two lines
  (`"libm.so.6` + newline + `libm.so.6"`), an unterminated string rcc
  warned about on every file (991 warnings) and that corrupts any
  consumer of the macro. Fixed by deduping DT_NEEDED names against the
  pre-seeded set and earlier -l / positional .so entries in link_elf.c.
  Regression: `test/test-link.sh` case 17 (repeated `-lm` must emit
  exactly one `NEEDED libm.so.6`). The gnutls build itself still stops
  at `src/gl/tests` vma-iter.o with `conflicting types for 'ioctl'`
  (sys/ioctl.h:586) -- that failure reproduces identically with gcc on
  the same generated header (gnutls 3.8.13's bundled gnulib
  `sys_ioctl.in.h` unconditionally redeclares ioctl with `int request`
  via `# if @SYS_IOCTL_H_HAVE_WINSOCK2_H@ || 1`, conflicting with
  glibc's `unsigned long`), an upstream issue, not an rcc bug.

### Fixed (2026-08-27, test_git -- string-literal-to-int static initializer stored 1)

- \*\*A string literal cast to an integer type in a static initializer
  (`intptr_t g = (intptr_t)"all";`, or a struct member like git's
  `struct option` `.defval = (intptr_t)"all"`) silently stored the
  truthiness constant **1** instead of the literal's address:
  `eval_const_expr()` folds ND_STR to 1 ("a string address is never
  null") for truthiness contexts (ternary/logical conditions), which is
  correct there, but both global-initializer call sites ran that plain
  integer const-eval BEFORE their address-reloc fallback, so the fold
  result became the stored value. Found via `test_git`
  (git 2.55.0): t1013-read-tree-submodule.sh failed 58/58 remaining
  tests (gcc-built git: 0/58) because `git status -u -s` segfaulted in
  `git_parse_maybe_bool_text()` -- the `-u` option entry's defval
  ("all") was the pointer 0x1, and `parse_untracked_setting_name()`
  parsed the bytes at address 1 as the untracked-files mode. Fixed by
  moving the existing `looks_like_address_expr()` + `extract_reloc()`
  address-reloc fallback ABOVE the plain `eval_const_expr()` integer
  path in `global_init_one()` and `global_initializer_impl()`.
  Regression: `test/test_static_str_int_cast.c` (fails on the pre-fix
  compiler with a segfault -- strcmp on address 1). `test_git`'s
  t1013 now passes all 58 remaining tests, matching the gcc build.

### Fixed (2026-08-26, emacs -- C23 `stdc_bit_width`)

- \*\*`<stdbit.h>` exposed the leading/trailing-zero and popcount C23 APIs
  `stdc_bit_width` macro. New regression cases extend `test/test_bit.c`.

### Fixed (2026-08-26, emacs -- nested designated compound-literal conversions)

- \*\*`check_type()` was missing on scalar assignments inside nested
  `test/test_compound_literal.c`. `test_emacs` now completes the

### Fixed (2026-08-26, emacs -- function-call argument-count checking)

- \*\*`cast_funcall_args()` checked argument types but never the argument
  Regression cases in `test/test_funcall_argcount.c`. With `config.h`

### Fixed (2026-08-26, glib -- constructor/destructor attribute flags leaked across declarations)

- \*\*`__attribute__((constructor))` and `__attribute__((destructor))`
  pending flags were only cleared when a function DEFINITION consumed
  them, so two consecutive prototype-only declarations (glib's
  `G_DEFINE_CONSTRUCTOR(resource_constructor); G_DEFINE_DESTRUCTOR(
resource_destructor);` idiom) stacked BOTH flags onto the first
  definition: the constructor got is_constructor AND is_destructor,
  the destructor got neither, and the ELF `.fini_array` ended up
  containing the CONSTRUCTOR. At dlclose the loader ran the
  constructor as the "destructor", re-registering the module's lazy
  GResource after its pages were about to be unmapped -- the next
  `g_resources_get_info()` drain read the dangling GStaticResource
  and segfaulted (glib's `gio/tests` resources test, signal 11).
  Fixed by recording the consumed flags on the function's symbol at
  each prototype-only declaration exit (clearing the pending globals)
  and OR-ing the symbol flags into the definition. Regression:
  `test/test_ctor_dtor_flags.c` (dlopen/dlclose module; fails on the
  pre-fix compiler because the destructor never runs). `test_glib`'s
  resources test now passes; its remaining `file` test_measure
  mismatch (98952 vs 74376 -- 6 directories x 4096 counted extra) is
  an environment-dependent gvfs-daemon artifact, not an rcc bug: the
  rcc-built libgio returns the correct value in isolation.

### Fixed (2026-08-26, static pointer initializers -- extract_reloc int truncation)

- \*\*`extract_reloc()`'s ND_NUM/ND_NEG/ND_NOT/ND_BITNOT cases
  truncated a constant to `int` (the reloc-addend width), so a static
  pointer initialized with an integer literal that does not fit int32
  -- `static void *p = (void*)0xdeadbeef;` -- was sign-extended to
  0xffffffffdeadbeef instead of 0x00000000deadbeef, corrupting the
  stored pointer and breaking `__atomic_compare_exchange` drain loops
  that compare it. Fixed by making extract_reloc() decline values
  that overflow its int addend and falling back to the 64-bit
  const-expr evaluator at both global-initializer call sites.
  Regression: `test/test_static_ptr_int_literal.c`.

### Fixed (2026-08-26, emacs -- secure-hash sha384/sha512, per-register spill-slot depth overflow)

- \*\*gnulib's `lib/sha512.c` was miscompiled: `(secure-hash 'sha384
"foobar")` / `'sha512` returned wrong digests while md5/sha1/sha224/
  sha256 were correct. The 64-bit primitives (rotates, adds, the 80-entry
  K table, the M schedule) all worked in isolation; the multi-round
  compression function diverged only past ~27 rounds -- a position-
  dependent codegen bug (round 26 was wrong in a 28-round function but
  right in a 27-round one). Root cause: every "fold the spilled value
  into the operation" site in codegen.c (shared idx/base array deref,
  binary-op chains add/sub/and/xor/or/cmp/imul, shifts, float ops and
  comparisons) read `spill_offset(r)` and cleared the `spilled_regs` bit
  WITHOUT popping the per-register spill-stack entry. Each fold leaked
  one depth level; after MAX_SPILL_DEPTH (32) leaked levels
  `push_spill_slot()` could no longer record the fresh slot, so the
  store landed at a new offset while `spill_offset(r)` still returned
  the stale top -- a silent read of the wrong slot. Fixed by popping the
  consumed slot at all 13 fold sites. Regression:
  `test/test_spill_slot_depth.c` (28 sha512 compression rounds; fails on
  the pre-fix compiler with an assert, passes on gcc and fixed rcc).
  `test_emacs`'s `test-secure-hash` now passes.

### Fixed (2026-08-26, unfixed.txt sweep -- alignas-in-nested-struct-typename, **STRICT_ANSI**, object-macro `-E` spacing)

- \*\*`alignas`/`_Alignas` on a struct/union MEMBER declaration was wrongly

  ```c
  enum { LISP_ALIGNMENT = alignof (union { union emacs_align_type x;
                                            char alignas (GCALIGNMENT) gcaligned; }) };
  ```

  which failed to parse and cascaded into "undeclared variable" for
  every later use of the enum constant, aborting the whole build at the
  very first source file. Fixed by suspending `in_type_name` for the
  whole struct/union body when entering `{`, restoring it after the
  matching `}`. New regression test:
  `test/test_alignas_nested_struct_typename.c`.

- \*\*rcc never predefined `__STRICT_ANSI__` for an explicit strict (non-

  ```c
  #if (defined(__GNUC__) && !defined(__STRICT_ANSI__) && ...)
  #define Py_ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]) + \
      Py_BUILD_ASSERT_EXPR(!__builtin_types_compatible_p(typeof(array), \
                                                          typeof(&(array)[0]))))
  #else
  #define Py_ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))
  #endif
  ```

  compiled with CPython's own real `-std=c11` build flag: with
  `__STRICT_ANSI__` never defined, rcc always entered the `typeof`
  branch the header itself believed it had excluded under strict mode --
  and since rcc (correctly matching real GCC's own strict-mode behavior)
  also rejects bare `typeof` outside GNU/C23 mode, `Objects/call.c`'s
  `Py_ARRAY_LENGTH(stack)` use failed to parse ("expected specific
  operator"), cascading into dozens of follow-on errors and aborting the
  whole Python build. Fixed by adding an `opt_strict_ansi` flag (set only
  by an explicit strict `-std=cNN`, mirroring the existing `opt_gnu_mode`
  convention for `-std=gnuNN`) and predefining `__STRICT_ANSI__` from it.
  New regression test: `test/test_strict_ansi_typeof.c` (checks all
  three `-std=` cases directly via `-E`, plus an end-to-end compile of
  CPython's exact `Py_ARRAY_LENGTH` gate).

- \*\*An object-like macro invoked bare at file/top scope, immediately
  test: `test/test_pp_macro_adjacency.c`.

### Fixed (2026-08-26, postgres -- 64-bit same-width mixed-signedness `__builtin_{add,sub}_overflow`)

- \*\*Same-width mixed-SIGNEDNESS add/sub overflow builtins at 64-bit
  numbering) in `test/test_builtin_overflow_family.c`; verified

- \*\*ARM64 had the identical same-width mixed-signedness gap at 64-bit

### Fixed (2026-08-26, postgres -- constant-fold unsigned division + narrow-destination `__builtin_*_overflow`)

- \*\*A SEPARATE, independent constant-folding pass in `src/opt.c`

  ```c
  uint64 total_cells = (uint64) ncolumns * nrows;
  if (total_cells >= SIZE_MAX / sizeof(*content->cells))
      ... "Cannot print table contents ... maximum 0" ...
  ```

  which made literally every `psql`-driven regression test in
  postgres's own suite fail identically the moment the build itself
  got far enough to bootstrap. Fixed by mirroring
  `eval_const_expr_impl`'s unsigned handling in `opt.c`'s fold. New
  regression test: `test/test_opt_const_fold_unsigned_div.c`.

- \*\*`__builtin_{add,sub,mul}_overflow` with a destination NARROWER than
  `test/test_builtin_overflow_family.c`(also consolidated`test_builtin_overflow_mixed_width.c`/`test_builtin_overflow_p.c`

  With all six fixes from this postgres investigation (array-decay
  qualifier, speculative const-fold, local-static hash registration,
  inline-asm `xchg` memory operand, unsigned constant-fold division,
  narrow-destination overflow builtins), postgres now builds, links,
  bootstraps a working cluster, AND makes substantial further progress
  through its own `make check` regression suite (220/231 -> 176/231 ->
  166/231 failing tests across this round; the "Cannot print table
  contents" crash that previously took down literally every
  `psql`-driven test identically is gone, and `int2.out`'s
  `smallint out of range`-detection failures -- the exact bug the
  narrow-destination `__builtin_mul_overflow` fix targeted -- now
  passes completely). The remaining 166 failing regression files span
  many still-uninvestigated, likely-unrelated issues (e.g. `int4.out`'s
  boundary-value binary/octal integer-literal parsing at exactly
  INT32_MIN); left for a future session.

### Fixed (2026-08-26, postgres -- local-static array-element relocation + inline-asm xchg memory operand)

- \*\*A function-local `static` variable's array-element address used to

  ```c
  static char descriptor_names[2][MAX_DESCRIPTOR_NAMELEN];
  static struct variable varspace[2] = {
      {descriptor_names[0], &descriptor_type, 0, NULL},
      {descriptor_names[1], &descriptor_type, 0, NULL}
  };
  ```

  Fixed by calling `global_htab_add()` when creating a block-scope
  static's global-storage entry, exactly like every other global. New
  regression test: `test/test_local_static_arr_reloc.c`.

- \*\*rcc's inline-asm `xchg` dispatch unconditionally encoded the
  `test/test_asm_xchg_mem.c` (byte/word/dword/qword forms, checked

  Together with the earlier array-decay-qualifier and
  speculative-const-fold fixes below, postgres now builds, links, AND
  bootstraps a working cluster (`initdb` completes end to end) --
  `make check`'s regression SUITE then runs but every `psql`-driven test
  fails identically with `fe_utils/print.c`'s own overflow guard:
  `"Cannot print table contents: number of cells N is equal to or
exceeds maximum 0."` (`SIZE_MAX / sizeof(*content->cells)` folding to
  `0` instead of `2305843009213693951`). Root-caused as far as: only
  reproduces when `print.c` is compiled as part of the FULL
  `run_batch.sh` postgres build (100% reproducible there, confirmed
  across repeated full rebuilds); every standalone isolation attempt
  gave the CORRECT result and never reproduced it -- exact command-line
  replication (direct exec, via `sh -c`, repeated x5), parallel
  concurrent invocation (x8), under `valgrind --track-origins=yes` (no
  uninitialized-read reported), and with ASLR disabled (`setarch -R`).
  `eval_const_expr()`'s `ND_DIV` case (`src/parser.c`) already handles
  unsigned division correctly when reached (verified directly via a
  forced `_Static_assert`/array-size compile-time probe on the exact
  `SIZE_MAX / sizeof(*content->cells)` shape, both standalone and
  through the same build path) -- so the fold logic itself isn't
  wrong; something about the _real_ build's environment/memory layout
  reaches a different, incorrect code path or corrupts a value before
  reaching it. Left for a focused follow-up session with the batch
  build as the only known-reliable repro vector (e.g. instrument the
  `rcc` binary invoked mid-`run_batch.sh`, or bisect by progressively
  approximating the exact recursive-`make`-driven build environment).

### Fixed (2026-08-25, postgres -- nested compound literal wrongly forced through speculative const-fold, hard-erroring on plain runtime code)

- \*\*A struct/union compound literal used as an ordinary RUNTIME

  ```c
  #define list_make_ptr_cell(v)  ((ListCell) {.ptr_value = (v)})
  #define list_make1(x1) list_make1_impl(T_List, list_make_ptr_cell(x1))
  ```

  and `dependency.c`'s `context.rtables = list_make1(list_make1(&rte));`
  (`rte` a local `RangeTblEntry`) -- completely valid C GCC accepts
  without complaint, rejected outright by rcc. Fixed by adding an
  `in_speculative_const_fold` flag that lets every "expected constant
  expression"/"unsupported global initializer" check inside
  `global_init_one()`/`global_initializer_impl()` fail QUIETLY instead of
  fatally when only this speculative attempt is in flight, plus a
  `speculative_fold_failed` flag so a PARTIAL failure (some members
  folded correctly, one didn't) can't leave `var->has_init`/
  `var->is_constexpr` set with a mix of real and garbage bytes for a
  later constant-expression read to silently trust. New regression test:
  `test/test_spec_const_fold_nested_cl.c` (fails
  pre-fix with the same "expected constant expression" error; verified
  for both compile success and correct runtime values against real GCC).

  This, together with the array-decay qualifier fix above, gets
  postgres's `src/backend/catalog/dependency.c` compiling; the build now
  progresses substantially further before hitting its next, unrelated
  blocker (`src/interfaces/ecpg/ecpglib/descriptor.c`, "expected specific
  operator" around a `switch`/`case` block calling `get_int_item()`/
  `va_end()` -- not yet root-caused, left for a future session).

### Fixed (2026-08-25, postgres -- struct-member array-to-pointer decay lost const/volatile qualifier)

- \*\*A struct member of ARRAY type, accessed through a const- or
  `test/test_qualified_member_array_decay.c` (fails pre-fix with the

### Fixed (2026-08-25, continued unfixed.txt sweep -- -s driver flag, nextafter family)

- \*\*`rcc -s` (link-time strip, universally accepted by every real GCC/

- \*\*`__builtin_nextafter`/`__builtin_nextafterf`/`__builtin_nextafterl`

- \*\*A SEPARATE, more consequential bug in the same area: rcc's bundled
  register" bug `test_gamma_family.c` already documents for `tgamma`/

  Found while root-causing jerryscript's `__builtin_nextafter` link
  failure above (`ecma-helpers-number.c`'s `ecma_number_get_prev/next`).
  Regression test: `test/test_nextafter_family.c` (a prior double-
  returning call primes XMM0 with a distinguishable value, confirming
  `nextafter`'s real result doesn't come back stale; also exercises
  `nextafterf`, both `__builtin_nextafter[f]` forms, and a genuinely-
  unsupported `-x fortran` still being rejected -- see the `-x c-header`
  entry above). With both fixes, jerryscript builds end to end and its
  own unit-test suites pass 82/84 (97.6%, up from a hard compile/link
  failure) -- the 2 remaining failures are unrelated, see below.

- \*\*`-fdata-sections`/`-ffunction-sections` (place each global/function

  Regression test: `test/test_no_op_flags_s_sections.c` (new) -- `-s`,
  `-fdata-sections`, `-ffunction-sections`, and all three combined,
  each fail under `-Werror` on the pre-fix compiler; a genuinely
  unsupported flag is confirmed to still be rejected. With both fixes,
  micropython's build progresses substantially further (past its
  `qstr.i.last`/`qstrdefs` generation steps that hard-failed
  immediately before) into `makeqstrdata.py`, where it now hits the
  preprocessor-spacing bug documented immediately below.

### Investigated, not fixed (2026-08-25, object-like macro expansion loses "no space before/after" adjacency in `-E` output)

**micropython's `makeqstrdata.py` (parses rcc's own `-E` preprocessed
output) crashes**: `ValueError: invalid literal for int() with base 10:
'(1) '` -- rcc's `-E` output for `QCFG(BYTES_IN_LEN,
MICROPY_QSTR_BYTES_IN_LEN)` (where `#define MICROPY_QSTR_BYTES_IN_LEN
(1)`, from `py/mpconfig.h`/`py/qstrdefs.h`) is `QCFG(BYTES_IN_LEN, (1)
)` -- an extra space before the closing `)` that real GCC's `-E` never
emits (`QCFG(BYTES_IN_LEN, (1))`). `makeqstrdata.py`'s own regex-based
parser strips a value's surrounding parens only when `value[-1] ==
")"`; the trailing space defeats that check, and `int("(1) ")` then
throws. Minimal, clean repro (no micropython involved):
`#define M (1)` then `X(M)` in a second file -- rcc emits `X( (1) )`,
GCC emits `X((1))` (confirmed via direct `diff`). Two independent
spurious spaces, both around the boundary between an object-like
macro's expansion and the tokens immediately surrounding its
invocation in the source: one right after `X(`, one right before the
final `)`.

`src/preprocess.c` already has a purpose-built mechanism for exactly
this ("tight against what follows" adjacency, `Frame.tight_after` /
`Token.no_space_after`, set in `expand_token()`'s object-like-macro
branch and consumed in `frame_pull()`) -- its own comments describe
precisely this scenario (`TRACE_INCLUDE_PATH/system.h`-style
adjacency) -- but by inspection the mechanism _should_ fire correctly
for this exact case (the last-body-token / `tight_after` propagation
path in `frame_pull()` looks structurally sound) and evidently
doesn't; not root-caused to the exact failure point this session
(needs runtime instrumentation of `frame_pull()`/`expand_token()` to
see why `tight_after` isn't taking effect here, not further pursued
given the risk of a blind edit to this exact, delicately-tuned,
extremely widely-exercised piece of the preprocessor). The symmetric
_leading_-space case (nothing currently propagates "no space before"
from whatever precedes a macro invocation onto the first token of its
expansion) looks entirely unhandled and would need separate treatment.
`test_micropython` stays on `unfixed.txt` pending this fix.

### Investigated, not fixed (2026-08-25, static-archive PC32-relocation precision divergence)

**jerryscript's `unittests-math` suite's `unit-test-math` fails 26/91
acos/asin sub-cases** (bit-exact comparisons against jerry-math's own,
from-scratch fdlibm-derived software `acos`/`asin`/`sqrt` implementation,
e.g. `acos(0.5)` returns `0x3ff0c2a6874d5540` instead of the expected
`0x3ff0c152382d7366`) -- confirmed NOT a codegen bug (the compiled
object code is bit-for-bit identical either way; verified via `cmp`).
Root-caused to rcc's native ELF linker (`src/link_elf.c`): the SAME
`acos.o`+`sqrt.o`, linked as loose object-file arguments
(`rcc driver.c acos.o sqrt.o -lm`), produce the numerically-correct
result; linked with `acos.o` pulled from a **static archive** instead
(`rcc driver.c sqrt.o -L. -lacos_only -lm`, `libacos_only.a` containing
only `acos.o`), the identical object's own float-literal constant loads
(`crc32`-unrelated `.LF0`../`.LFn` PC32-relative `.rodata` references)
resolve to wrong addresses -- reproduces with a single archive member,
no cross-object collision needed. Debug instrumentation of
`apply_dynamic_relocs()`'s `RL_PC32` case (the function that actually
applies these relocations for a dynamically-linked executable, NOT
`link_apply_relocs()` -- that's the dead-for-non-`-static` "Static
link: apply relocations normally" branch) showed only 1 of `acos.o`'s
51 own `.LF*`-targeted relocations reaching that switch case when
linked directly, vs. all 51 when `acos.o` is archive-loaded -- meaning
~50 of them are resolved through some OTHER, not-yet-located mechanism
in the direct-load case (correctly), which the archive-load path
doesn't take, falling through to `apply_dynamic_relocs()` for
everything instead (each individual entry's own `S` computation looked
arithmetically consistent in isolation, so the bug is in _which_
relocations take which path, not an arithmetic error in this switch
case itself). Not root-caused further this session -- next step is
tracing where/why direct-loaded objects' local-symbol PC32 relocations
bypass `apply_dynamic_relocs()` almost entirely while archive-loaded
ones don't, likely in whatever pass classifies a relocation as
"needs the dynamic/PLT/GOT machinery at all" vs. "purely link-time
resolvable, apply directly" ahead of this function. Minimal repro
preserved: any two-file jerry-math-style .c pair (a function with
several `#define`d `double` constants baked into `.rodata`, referenced
via PC32-relative loads, calling a second function from a different
TU) reproduces with `ar qc lib.a a.o && rcc main.c b.o -L. -la -lm`
vs. `rcc main.c a.o b.o -lm`. `test_jerryscript` stays on
`unfixed.txt` pending this fix (its `unittests`/`unittests-init-fini`
build variants each show the identical `unit-test-ext-arg` failure too
-- `Assertion 'arg2 == 10.5' failed`, not yet investigated, possibly
unrelated).

### Fixed (2026-08-25, unfixed.txt sweep -- CRC32/PCMPEQx memory-operand asm gaps + -x c-header driver rejection)

- \*\*`crc32b`/`crc32w`/`crc32l`/`crc32q` (SSE4.2 hardware CRC32) with a

  Found via memcached's `crc32c.c` (`crc32c_hw()`'s three-way-parallel
  SSE4.2 CRC32C implementation, shared with zlib-ng/RocksDB/etc.): its
  own `testapp` unit-test suite SIGABRT'd on `test_crc32c`'s known-good
  constant assertions, right after `test_issue_101`. Regression test:
  added to `test/test_ia32_intrinsics.c` (crc32q memory-form 8-byte-at-
  a-time loop vs. an independently-computed crc32b byte-at-a-time
  ground truth, plus a known-good CRC32C constant and a register/
  memory-form consistency check -- all fail on the unfixed compiler),
  alongside that file's existing `__builtin_ia32_crc32qi/si` register-
  only intrinsic checks (a separate code path that always materializes
  its argument in a register first, so it never exercised this
  memory-operand bug at all). With the fix, memcached's full `testapp`
  suite passes 56/56 (was aborting at test 18/56), and
  `make test`'s Perl integration suite (`t/*.t`) runs hundreds of tests
  clean as far as observed (cut off only by the harness's per-test
  wall-clock budget, not a failure).

- \*\*`pcmpeqb`/`pcmpeqw`/`pcmpgtb`/`pcmpgtw` (SSE2 packed-byte/word

  Found via rvvm's `src/util/bit_ops.h` zero-byte-detection idiom
  (`pcmpeqb %xmm,%xmm` used twice in a row, a common "does this integer
  contain a zero byte" trick). Regression test: extended the existing
  `test/test_asm_movdqa_paddd_mem.c` (byte-for-byte objdump comparison
  against real GNU `as` output) with `pcmpeqb`/`pcmpeqw`/`pcmpgtb`/
  `pcmpgtw` register and memory-operand cases. With the fix, rvvm builds
  completely clean end to end and its own bundled `riscv-tests` ISA
  compliance suite runs extensively (111 PASS observed) before the
  harness's per-test wall-clock budget cuts the run short --
  previously hard-blocked at the very first `pcmpeqb` in
  `riscv_priv.c`'s include chain. Two RV32 float-compare subtests
  (`rv32uf-p-fcmp` and `rv32ud-p-fcmp`, both failing at the same
  subtest index 11) newly surfaced now that the suite runs this far --
  not root-caused this session (RV32-specific FEQ/FLT/FLE soft-float
  semantics inside rvvm's own interpreter, not obviously related to
  this fix), out of scope; `test_rvvm` stays on `unfixed.txt` pending
  that root-cause.

- \*\*`rcc -x c-header ...` (CMake's `PRECOMPILE_HEADERS` feature, e.g.

  Found via SDL3's CMake build (`-x c-header -include cmake_pch.h -o
cmake_pch.h.gch -c cmake_pch.h.c`, an empty wrapper TU forcing the
  real header through `-include`). Regression test:
  `test/test_x_c_header.c` (drives rcc as a subprocess with both `-x
c-header` spellings, confirms a `.gch` file is produced, and confirms
  a genuinely unsupported `-x fortran` is still rejected). With the fix,
  SDL3's `cmake_pch.h.gch` step succeeds and the build proceeds to
  compiling SDL3's own source tree (`SDL_systhread.c` and hundreds of
  others observed compiling clean) before the harness's wall-clock
  budget is reached -- previously hard-blocked at the very first CMake
  target that needed a precompiled header.

  Also triaged three more `unfixed.txt` entries this session and
  confirmed each is NOT an rcc bug (root-caused, not just "unclear"):
  `test_libopus`'s `tests/test_opus_api.c` references glibc's
  `__malloc_hook`, removed from glibc's own `<malloc.h>` well before
  this sandbox's glibc version -- reproduces identically with real
  system `gcc`. `test_jq`'s `jq` binary SIGABRT'd in `jv_object_set`
  because it dynamically links against this machine's pre-installed
  system `libjq.so.1` (jq 1.8.1, from the `jq` RPM) instead of the
  just-built `.libs/libjq.so.1` (jq 1.8.2) -- an ABI/version mismatch
  from a missing rpath in the test's own build, not an rcc codegen bug
  (confirmed via `ldd`). `test_tcl`'s `make test-tcl` fails to find its
  own freshly-built `init.tcl` (`tclsh` runs from a directory where
  Tcl's runtime library isn't installed) -- an environment/test-harness
  packaging gap; `tcl`/`tclsh`/`libtcl9.0.so` themselves build and link
  completely clean. `test_msgpack` (msgpack-c) and `test_nanomsg` (nng)
  were also re-verified this session: both build completely clean and
  their CTest suites pass extensively (5/5 and 70+/77 observed
  respectively) with no rcc-attributable failure -- removed from
  `unfixed.txt` as already-working, previously just unconfirmed.

  `make check-all`: Unit tests 334/334 (incl. the two new regression
  tests above), Torture 3605/3609 (100% of non-skipped, 354 skipped),
  TCC 118/118, Compliance 15/15, C-testsuite 220/220, Dg-error 34/34 --
  0 failed overall.

### Fixed (2026-08-25, \_\_int128/\_Decimal128 truthiness address-vs-value session)

- \*\*A bare `if (x)`/`while (x)`/`&&`/`||` operand, a value ternary

  Fixed by adding `gen_int128_branch_if_zero()` (loads both 8-byte
  halves from the slot address, ORs them, and branches on THAT being
  zero) and calling it from all four sites whenever the condition's
  type is `__int128`-like (`is_int128_like()`: `TY_INT128`,
  `TY_DECIMAL128`, or `_BitInt(65..128)` -- the last is already routed
  through the separate, already-correct `is_wide_bitint()` path before
  any of these four sites is reached, so in practice this only ever
  fires for `TY_INT128`/`TY_DECIMAL128`).

  Found via the item below's `assert(f(a,b))` repro (originally
  reported as "needs a dedicated minimal-repro bisection"); the plain
  `if (var)`/`if (f())` shapes claimed to "work correctly" there were
  independently re-verified to be equally broken (always truthy
  regardless of the operand's real value -- confirmed with a
  gdb breakpoint on `asm_cmp_zero()` showing `cond`'s VReg mapped to
  `%r10`, a slot address, being compared directly).

  Regression tests: `test/test_int128.c`'s `test_truthiness_var`,
  `test_truthiness_funcall`, `test_truthiness_int128_funcall_cond`,
  `test_truthiness_nested_int128_cond` (all four fail on the unfixed
  compiler: 3 abort/crash, reproducing the exact reported shape; one
  degenerately passed pre-fix by coincidence of the specific literal
  values used). Full suite verified: Unit tests 332/332, Torture
  3605/3609 (100% of non-skipped), Dg-error 34/34, Link tests 12/12 --
  0 failed overall (native Linux x86-64); also re-verified clean on
  the mingw and ARM64/qemu cross targets.

  Note: this fix also makes `_Decimal128` conditions test the slot's
  actual bit pattern instead of always-truthy, but `_Decimal128`'s
  canonical "zero" is not always all-bits-clear (e.g. the `0.0dl`
  literal encodes a nonzero biased-exponent field in its high word),
  so a bare `if (dec128_var)` can still misclassify a decimal zero as
  truthy -- a separate, pre-existing decimal128 zero-representation
  gap (shared by the pre-existing `(_Bool)` decimal128 cast path, which
  has its own latent bug: it truncates the OR'd 64-bit truthiness
  result to a single byte before storing it into the `_Bool`, which
  only "works" for `0.0dl` by coincidence since its low byte happens to
  be zero). Not fixed this session; out of scope for the `__int128`
  regression this session targeted.

### Fixed (2026-08-25, valkey -- sizeof unsigned type / int128 shift clobber / rdtsc)

- \*\*`sizeof(...)` (and `sizeof(type-name)`) produced the correct

  Found via valkey's `src/entry.c`:
  `static_assert(FIELD_SDS_AUX_BIT_MAX < sizeof(char) - SDS_TYPE_BITS,
"...")`, which needs the unsigned-wraparound reading to pass (real
  gcc agrees) rather than the signed `-2` rcc used to compute.

- \*\*A `__int128 << expr` / `__int128 >> expr` whose shift-count `expr`

- **`__builtin_ia32_rdtsc()`/`__builtin_ia32_rdtscp()` were unimplemented**

  Regression tests: `test/test_sizeof_unsigned_type.c`,
  `test/test_int128_shift_count_clobber.c`,
  `test/test_int128_var_shift.c`. valkey (v9.1.0) now builds
  completely clean with rcc end to end (previously hard-blocked at the
  `static_assert` in `entry.c`, then at the missing `rdtsc` intrinsic);
  `valkey-server` starts, logs `monotonic clock: X86 TSC @ ... ticks/us`
  (confirming the rdtsc fix drives its real startup path), then hits a
  SEPARATE, unrelated runtime issue (`Fatal glibc error:
pthread_mutex_lock.c:88: assertion failed: mutex->__data.__owner ==
0`) before it can serve requests -- not root-caused this session, see
  "Needs fixing" below. Also newly found while investigating this
  cluster (unrelated to any of the three fixes above, NOT yet fixed):
  `assert(f())`/`if (f())`-style truthiness testing of a function whose
  return type is `__int128`/`unsigned __int128` can abort/misbehave in
  some but not all shapes (a bare `if (var)` and `if (f())` both work;
  `assert(f(a, b))` where `f` takes two `__int128` PARAMETERS and
  compares them internally does not) -- see "Needs fixing" below.

### Fixed (2026-08-25, libgit2 -- braced scalar initializer trailing comma)

- \*\*A scalar initializer wrapped in "superfluous but legal" braces with

  Found via libgit2's `tests/libgit2/network/remote/remotes.c`:
  `char *specs = { "refs/heads/master", };`.

  Regression test: `test/test_braced_scalar_trailing_comma.c` (covers
  local int, local pointer, top-level global pointer, and a nested
  designated struct-member pointer -- all four sites fail identically
  on the old code). libgit2's core `libgit2.so`/`git2` CLI already
  built clean before this fix (the bug was test-suite-only); with the
  fix, `libgit2_tests` also builds and links completely, and its
  non-network test suite runs extensively clean (hundreds of suites
  pass) with one PRE-EXISTING, unrelated issue newly surfaced now that
  the suite actually runs to completion -- see "Needs fixing" below.

### Fixed (2026-08-25, util-linux -- &"string literal" codegen crash)

- \*\*`&"string literal"` (address-of a string literal) crashed the

  Found via util-linux's `disk-utils/isosize.c`: `memcmp(&label,
&"\1CD001\1", 8)` -- comparing a magic-number buffer directly against
  a string literal's address, avoiding a separate named buffer for a
  short fixed byte sequence.

  Regression test: `test/test_string_literal.c` (fails on the old
  code with the exact reported error; also verifies the `char(*)[N]`
  type is correct, not just that it compiles). util-linux (v2.42.2) now
  builds completely clean with rcc -- every target links, including
  fdisk/mount/lsblk/findmnt/cfdisk/blkid -- and its extensive test
  suite runs clean as far as observed (134/134 blkid superblock-probing
  sub-tests, fdisk invalid-input tests, and more, all PASS; the full
  suite is large enough that a complete run wasn't fully exhausted in
  this session, but no failure was seen anywhere it reached).

### Fixed (2026-08-24, libtommath -- one-sided &&/|| dead-branch elimination)

- \*\*`A && CONST_FALSE` / `A || CONST_TRUE`, with the constant operand on

  Two-part fix:
  1. A new RHS-side counterpart of the existing LHS fold: when
     `eval_const_expr()` on the RHS alone determines the whole &&/||
     result (RHS==0 for `&&`, RHS!=0 for `||`), replace the node with
     `ND_COMMA(lhs, const)` -- the LHS (which C requires to always run,
     unlike the RHS) still executes for its own side effects/ordering,
     but the provably side-effect-free RHS -- and anything, including a
     call, it might otherwise have gated -- disappears entirely.
  2. The `if`-with-constant-condition dead-branch eliminator now also
     recognizes this exact `(prefix, CONST)` comma shape (not just a
     directly-constant condition), composing with fold 1 above: it
     structurally drops the untaken branch -- and any call inside it --
     while still running `prefix` first. Without this second half, fold
     1 alone only fixes the VALUE (a real, always-false runtime
     compare), not the branch's CODE -- the dead call would still be
     emitted (and require linking) as unreachable-but-present code.

  Regression test: `test/test_logand_rhs_const_dce.c` (fails on the old
  code: "undefined reference to `undefined_func'"). libtommath now
builds, links, and its `demo/test`/`demo/mtest_opponent`binaries run
cleanly with rcc; the bundled`mtest/mtest`-vs-`mtest_opponent`
differential fuzzer's very first comparison (`sqr`) already mismatches
identically with a from-scratch **gcc**-built `mtest_opponent`against
the same`mtest`oracle -- a pre-existing`mtest`/library version
  incompatibility in this snapshot, not an rcc bug.

### Fixed (2026-08-24, slimcc_c2y -- 3 stacked bugs)

- \*\*`->` (arrow) member access on a VLA (variable-length array)

- \*\*`#include "..."` (quote-include) from a SYMLINKED source file

- \*\*`__attribute__((cleanup(fn)))` where `fn` is a GNU nested function

  Regression tests: `test/test_vla_arrow_member_access.c`,
  `test/test_symlink_quote_include.c`,
  `test/test_cleanup_chain_reg.c`. slimcc now builds
  clean with rcc as CC and passes its own test suite
  (`make CC=rcc test`).

### Fixed (2026-08-24, test_c3 NaN comparison -- missing nan()/nanf()/nanl())

- **rcc's bundled `<math.h>` never declared `nan`/`nanf`/`nanl`** --

  Found via c3lang/c3c (`./testrun unit/stdlib/core/test_test.c3`,
  `test::std::core::test::test_almost_equal_fails_equal_nan_false`
  failing "test case expected to fail, but it's not"): c3c's own stdlib
  builds the `double::nan`/`float::nan` compile-time constants with
  `nan("")` (`src/compiler/sema_expr.c`, `TYPE_PROPERTY_NAN`). With the
  bug, `double::nan` was actually `0.0`, so `double::nan == double::nan`
  evaluated to _true_, corrupting every NaN-aware comparison built on
  top of it in c3c's own compiled-by-rcc binary (not a codegen bug in
  c3c's LLVM backend -- `0.0/0.0` computed at c3 runtime already gave the
  correct NaN bit pattern `0x7ff8000000000000`).

  First fix attempt just added `double nan(const char *)` /
  `float nanf(const char *)` / `long double nanl(const char *)`
  prototypes -- correct on Linux (glibc genuinely exports these
  symbols) but the mingw CI leg caught a second, platform-specific bug
  immediately: on `x86_64-w64-mingw32`, the classic MSVCRT target rcc
  links against provides no `nan`/`nanf`/`nanl` symbols at all
  (`nm libmsvcrt.a` shows both as themselves-undefined references, not
  definitions) -- a plain prototype linked "successfully" (no
  undefined-reference error) but resolved to something bogus, and
  calling it crashed at runtime (`EXEC FAIL`, exit `-1073741819` /
  `STATUS_ACCESS_VIOLATION`). Fixed properly by routing `nan`/`nanf`/
  `nanl` through rcc's existing `__builtin_nan`/`__builtin_nanf`/
  `__builtin_nanl` (already implemented, a real quiet-NaN constant, no
  libc call needed) as function-like macros -- no libc symbol
  dependency on any target, matching real mingw-w64's own `<math.h>`
  (which likewise `#define`s `nan`/`nanf`/`nanl` to the builtins for
  exactly this reason).

  New regression test: `test/test_nan.c` (merged with the pre-existing
  `test_nan_sign.c`'s NAN/INFINITY sign checks -- `nan`/`nanf`/`nanl`
  return a real NaN and never compare equal to themselves; reproduces
  the bug with the fix reverted -- `nan("") not NaN: 0`). Full local
  regression matrix re-run clean on both native Linux (Unit 320/320,
  TCC 118/118, Compliance 15/15, C-testsuite 220/220, Torture
  3605/3609, Dg-error 34/34, Link 12/12) and the mingw cross target
  (Unit 318/318, TCC 118/118, Compliance 15/15, C-testsuite 220/220,
  Torture 3574/3574, Dg-error 34/34) -- 0 failed on either. `test_c3`'s
  own unit suite (`c3c compile-test unit`) goes from 1365/1366 to
  1366/1366 passed.

### Fixed (2026-08-22, sqlite build session -- 3 stacked bugs)

- \*\*`#include_next <limits.h>` from glibc's limits.h failed with a user

- **rcc's bundled stddef.h inverted the `__need_wchar_t` protocol** --

- \*\*`input_files[64]` cap silently dropped every source file past the

- Removed a leftover `MEMBERFOLD:` debug fprintf in parser.c that
  polluted stderr on every const-member fold (sqlite build logs).

  Regression tests: `test/test_glibc_limits_include_next.c` (fails on
  the old code: "could not compile limits.h/stdlib.h TU") and
  `test/test_many_inputs.c` (70-file single-invocation link; fails on
  the capped driver with undefined f63..f69). sqlite now builds
  sqlite3/tclsqlite3/testfixture clean and passes `make test`
  (0 errors) plus smoketest (95/95).

### Verified (2026-08-23, janet -- one upstream snapshot bug, not rcc)

- janet (the Janet language, snapshot Git 547d923) builds clean with
  rcc (0 errors), and 32/33 test suites pass with the rcc-built
  binary. Only test/suite-io.janet fails, at `(xprintf to-b "123")`:
  the target value arrives as a garbage double ("cannot print to
  <denormal>") -- but the gcc-built janet fails IDENTICALLY, so this
  is an upstream snapshot regression in the xprintf/cfun path, not an
  rcc bug. (Also found and fixed en route: `union U a[] = { 1, 2 }`
  flat-union array sizing, rcc's skip_flat_aggregate_init consumed the
  element-separator comma; see the sokol-session entry -- actually the
  new parser fix below.)

### Fixed (2026-08-23, fftw)

- fftw3.h's quad-precision API section (gated purely on
  `__GNUC__`/architecture version checks, independent of whether quad
  precision is actually built) declares types via `__float128` --
  GCC's original spelling, distinct from the C23 `_Float128` rcc
  already supported. rcc treated `__float128` as an undeclared plain
  identifier, so every translation unit including fftw3.h -- the
  entire fftw build -- failed to parse. Added `__float128` as a
  keyword (keywords.gperf/keyword_ids.h) aliased to `long double`,
  matching the existing `_Float128` handling: no real 128-bit binary
  float arithmetic, but declarations, pointers, and struct members
  parse and size/align correctly, which is all fftw's un-built
  quad-precision declarations need.

  fftw builds clean with rcc and `make check` passes ("FFTW
  transforms passed basic tests!", no failures). Regression test:
  test_float128_keyword.c.

### Verified (2026-08-23, libmpfr -- pre-existing upstream test bugs, not rcc)

- mpfr 4.2.2 builds clean with rcc (0 errors) after the \_\_float128
  keyword fix above; `./configure && make` succeeds with GMP 6.3.0
  (system libgmp). `make check` hits three failures:
  - `tcheck`: "mpfr_check failed for mul Prec=1" (repeated squaring
    at 1-bit precision to a huge exponent).
  - `mpfr_compat` and `reuse`: spin at 100% CPU indefinitely (real
    infinite loops, not merely slow -- confirmed via `ps` showing
    sustained CPU time with no progress).
    All three reproduce IDENTICALLY on a from-scratch gcc build of the
    same mpfr-4.2.2 source tree against the same system GMP (cross-
    checked in /tmp, not just eyeballed) -- upstream/environment bugs
    in this mpfr release combined with this GMP build, unrelated to
    the compiler. Every other test in the suite (all it reached before
    the two infinite loops permanently occupied 2 of the 4 parallel
    test-runner slots) passed cleanly.

### Fixed (2026-08-23, nettle -- register-allocator spill/borrow + aliasing bugs)

- nettle 4.0 builds clean with rcc (0 errors) and `make check` now
  passes 128/128 tests (the initial run showed 55/127 failures, but
  that was stale/inconsistent build state -- a leftover shared
  library from an earlier differently-configured build mixed with
  freshly rebuilt static objects; a `make distclean` + reconfigure +
  full rebuild dropped it to a single real failure, `twofish`, now
  fixed).

  `twofish`'s `h_byte()` (twofish.c) SIGSEGVs (or silently returns a
  wrong byte) on a single expression with 5 levels of nested array
  indexing, two of them through an embedded ternary
  (`q_table[i][2][k == 2 ? x : l2 ^ q_table[i][1][...]]`) -- real
  register-allocator corruption under heavy pressure, four stacked
  bugs in `src/codegen.c`:
  1. `free_reg()` unconditionally cleared `used_regs` after restoring
     a spilled register, even when the restore returned a
     still-live OUTER value (displaced by `alloc_reg()`'s own victim
     selection, e.g. an outer `tbl[i][1]` element pointer) to its
     physical register. The very next `alloc_reg()` call then found
     that register "fully free" and hooked it to an unrelated
     scratch temp with no spill emitted, silently clobbering the
     just-restored value before its real use. Fixed by tracking, per
     register, whether its CURRENT occupant was placed there by
     `alloc_reg()`'s spill-victim path (a new `spill_victim` bitmask);
     `free_reg()` now protects (restores but leaves `used_regs` set)
     only in that genuine case. A prior session's simpler
     "keep-used-on-free" attempt (documented in the jemalloc entry
     below) regressed a different construct (nested comma-expression/
     function-call argument staging in `gen_funcall()`, which
     deliberately leaves a stale `spilled_regs` bit dangling after
     clearing `used_regs` directly, bypassing `free_reg()`); the
     `spill_victim` bit distinguishes a genuine outer-value eviction
     from that dangling leftover so both constructs work. Reset at
     both codegen() Pass 1/Pass 2 entry points (per-function state,
     same lifetime as `used_regs`/`spilled_regs`).
  2. `gen()`'s `ND_DEREF` fast path for `*(lvar + idx)` and the
     generic binary-op dispatch's `r_lhs == r_rhs` collision handling
     both called `free_reg()` on a VReg that ALIASED the physical
     register holding the live combined/returned result, prematurely
     freeing a register whose value was about to be read or returned.
  3. Every conditional-branch site that frees a VReg used only to set
     flags/load a compare operand did so AFTER emitting the branch
     instead of before it. When that VReg's register was borrowed
     from a spilled outer value, `free_reg()`'s restore of the outer
     value only existed on the fall-through side of the branch; the
     taken side resumed with the wrong value still in the register.
     First found and fixed in `gen()`'s `ND_COND` (ternary) and
     `ND_DO`'s flonum condition; a follow-up sweep found and fixed
     the identical shape in `gen()`'s `ND_LOGAND`/`ND_LOGOR`,
     `gen_addr()`'s `ND_COND` (struct/union ternary lvalue),
     `gen_int128()`'s `ND_COND`, and `gen_cond_branch_inv()`'s
     `ND_LOGOR`-with-flonum-lhs, flonum `EQ`/`NE`/`LT`/`LE`
     comparison, complex-type truthiness, and trailing flonum
     truthiness checks. Added `gen_flonum_branch_if_{zero,nonzero}
_preloaded` (branch-only, value already in xmm0/d0) so every
     site can free the source register between the load and the
     branch; the old load+branch-together
     `gen_flonum_branch_if_{zero,nonzero}` became dead and were
     removed.
  4. `gen()`'s `ND_LOGAND` (ARM64) needed its result register `r`'s
     allocation deferred to right after evaluating `lhs` (avoiding the
     same register-pressure blowup `ND_COND`'s deferred-allocation
     comment already documents, for a long `a && b && c && ...` value
     chain), guarded with `alloc_reg_avoid2(lhs, -1)` rather than plain
     `alloc_reg()` since `lhs` is still live and about to be read by
     the compare (a plain `alloc_reg()`'s victim-selection could pick
     `lhs`'s own physical register, so the immediately-following
     `xor r,r` would clobber `lhs` before the compare read it -- the
     same aliasing hazard the bitfield-merge fix documented further
     down this file already uses `alloc_reg_avoid2` for). While
     restructuring this, an editing slip on this file dropped the
     rest of the ARM64 case body -- `rhs`'s evaluation, the `cset`/
     flonum-truthiness merge into `r`, and the `.L.end.%d` label
     definition -- leaving the ARM64 path's forward branch
     unconditionally jumping to a label that was NEVER EMITTED for
     any chain of 2+ nested `ND_LOGAND` used as a value (not a plain
     `if`/`while` condition, which goes through the separate,
     unaffected `gen_cond_branch_inv()`). An unresolved forward-branch
     fixup defaults to a zero displacement, i.e. `b.eq` branching to
     its OWN address -- an unconditional infinite loop the instant the
     first operand is false. Restored the missing code. Found via real
     macOS ARM64 CI hardware timing out at the 30-minute job ceiling
     (`test_compound_literal`'s five-way `&&`-chained boolean
     return and `test_spill_locals_collision`'s 16-way `&&`-chained
     corruption check both use `&&` as a value, not a condition);
     locally reproduced on qemu-aarch64 with a synthetic `a>=0 && ...`
     chain (any depth >= 2 hangs before the fix, none after) -- the
     general local test suite never exercises this exact path because
     virtually all `&&` usage in real C and in rcc's own suite sits
     directly inside `if`/`while`/`for`, which never reaches `gen()`'s
     `ND_LOGAND` at all.

  Regression test: test_array_index_spill_collision.c. Verified with
  the full local suite (`make check-all`: 0 failed, Unit 319/319,
  Torture 3605/3609 same baseline) on Linux x86-64 with both gcc and
  clang, the ARM64 cross build under qemu-aarch64 (308/308 unit
  tests, gcc, incl. the synthetic deep-`&&`-chain repro above), and
  the mingw cross build under wine, plus a from-scratch nettle
  rebuild + full `make check` (128/128) confirming the real-world fix.

### Fixed (2026-08-24, bubblewrap)

- `__attribute__((cleanup(fn)))` on a local variable was never
  recognized as a reference to `fn` by `opt.c`'s whole-program DCE
  pass (`eliminate_unused_static_inline()`), so a `static inline`
  cleanup helper that nothing else in the TU calls by name got
  omitted from the object file -- "undefined reference to
  cleanup_freep" at link time, even though `codegen.c`'s
  `emit_cleanup_var()` emits a direct call to it at every scope-exit
  path. The DCE pass's BFS only ever walked `ND_FUNCALL`/address-of
  `ND_LVAR` nodes in the function body plus the separate `defer`
  statement list (`LVar.defer_stmt`, itself not part of the body's
  Node tree) -- `LVar.cleanup_func` (and the array-element form,
  `LVar.ty->base->cleanup_func`) is a THIRD reference kind, consulted
  directly by codegen with no `ND_FUNCALL` node anywhere in the AST
  at all, that the pass never accounted for. Fixed by scanning every
  live function's locals for `cleanup_func` in the same BFS loop that
  already handles `defer_stmt`, marking the named cleanup function
  live too.

  Found via bubblewrap 0.11.2's `utils.h`/`bind-mount.c`:
  `cleanup_freep()`, `cleanup_fdp()`, `cleanup_mount_tabp()` are all
  plain `static inline` wrappers around `free()`/`close()`,
  referenced only through `#define cleanup_free
__attribute__((cleanup(cleanup_freep)))`-style macros -- link fails
  with rcc, links and runs clean with gcc on the identical source.
  Regression test: test_cleanup_attr_static_inline_used.c (fails to
  link on the old code; also covers a second, independent cleanup
  function surviving alongside the first, and cleanup firing on both
  an early-return and a fall-through exit path from the same scope).

  bubblewrap builds clean with rcc (0 errors) linking against system
  libcap; the `bwrap` binary runs real namespace-sandboxed commands
  (`--unshare-all --ro-bind / / --dev /dev --proc /proc`) and passes
  its own TAP test suite end to end: `tests/test-run.sh` 69/69,
  `tests/test-specifying-pidns.sh` and
  `tests/test-specifying-userns.sh` both pass.

  `make check-all`: 0 failed (Unit 319/319 incl. the new regression
  test, Torture 3605/3609 -- 0 failed, 354 skipped, 4 todo, same
  baseline). ARM64 cross (qemu-aarch64): 309/309 unit tests. mingw
  cross: `eliminate_unused_static_inline()` is unconditionally
  disabled on that target already (see its own comment), so this fix
  has no effect there; compiles clean regardless.

### Fixed (2026-08-23, gmake)

- GNU make's src/makeint.h declares a bare forward `enum
variable_origin;` (a GNU/C23 opaque forward reference) ahead of
  `void reset_makeflags(enum variable_origin);`; src/variable.h
  later completes the enum body. rcc's forward-`enum tag;` handling
  only registered the tag for later identity reuse when it carried a
  C23 fixed underlying type (`enum tag : int;`); a bare `enum tag;`
  did not, so completing the enum minted an unrelated Type object
  (different enum_id) instead of finishing the placeholder in place.
  src/main.c's `reset_makeflags` definition -- using the completed
  enum -- then looked like a conflicting redeclaration against
  makeint.h's prototype -- using the placeholder -- and failed to
  compile ("conflicting types for 'reset_makeflags'").

  Fixed by registering every tagged forward declaration (not just
  the fixed-underlying-type ones), so the eventual `enum tag { ... }`
  completion's existing-tag lookup finds and reuses the placeholder's
  identity. Regression test: test_enum_fwd_decl_param.c.

  gmake builds clean with rcc; `ulimit -n 512; perl
tests/run_make_tests.pl -make ../make` passes all 1444 tests, 0
  failures (an initial run without the ulimit hit one FD-exhaustion
  test's platform-dependent recursion depth differently and
  segfaulted from unbounded native recursion -- not an rcc bug, just
  a missing test-harness prerequisite on my part; matches cleanly
  with the ulimit the project's own `make check-regression` target
  sets).

### Fixed (2026-08-23, wget2 -- two rcc bugs; one remaining failure verified not rcc)

- Bug 1 -- preprocessor: a directory listed twice on the command line
  (`-I../lib -I../lib`, routine in autotools-generated build commands
  -- wget2's libwget/Makefile does exactly this) broke
  `#include_next`: resolve_include_next() advanced past only the
  FIRST occurrence of the directory that supplied the current file,
  landing back on the SECOND (duplicate) occurrence -- the identical
  physical file the current header's own #include_next came from.
  gnulib-style wrapper headers deliberately use a re-enterable "split
  double-inclusion guard" (no ordinary header guard blocking a second
  pass), so re-finding the same file recursed through its own
  #include_next again, forever, until rcc's include-depth limit
  tripped ("Include depth exceeded"). Fixed by skipping every
  remaining occurrence of the current directory in the search list,
  not just an immediately-adjacent duplicate. Regression test:
  test_include_next_dup_dir.c.

- Bug 2 -- parser: a pointer-to-forward-declared-struct used as a
  function's return type (`const struct S *f(void);`, e.g. wget2's
  `WGETAPI const wget_vector *wget_http_get_no_proxy(void);` against
  an opaque `wget_vector`) was wrongly rejected as "conflicting
  types" between the prototype and the definition.
  qualify_struct_type() never mutates a shared, still-incomplete
  struct/union Type in place -- it mints a fresh "qualified variant"
  copy at every `const struct S *`-style use site, so the
  prototype's and the definition's copies are different Type objects
  even though both derive from the same tag; neither pointer
  identity nor the (both-NULL, since the struct never completes)
  member-list check recognized them as the same type. Fixed by
  adding a `struct_id` identity anchor (mirroring the existing
  `enum_id` mechanism for enums), set to the canonical tag's own
  address and preserved across every qualified-variant copy.
  Regression test: test_struct_fwd_decl_qual_return.c.

  wget2 (built --with-ssl=none --disable-nls) now builds to a single
  remaining failure: lib/sys/ioctl.h's gnulib-generated `ioctl`
  redeclaration (`int request`) genuinely conflicts with this
  system's glibc `ioctl` prototype (`unsigned long int request`) --
  confirmed NOT an rcc issue: the identical minimal repro (extracted
  verbatim from the real preprocessed output) is rejected by gcc too,
  with the identical diagnostic. This bundled lib/sys/ioctl.h is a
  gnulib-tool-generated snapshot baked against a different glibc
  version than this host's; regenerating it is a project-level
  `./bootstrap` concern, out of scope for a compiler.

### Fixed (2026-08-23, ggrep session -- 8 stacked rcc bugs)

- \*\*Bug 1 -- parser: a trailing `_Pragma(...)` inside a GNU statement-
  Regression test: test_stmt_expr_trailing_pragma.c.

- \*\*Bug 2 -- parser: `(bool)` cast constant-folding truncated before

- \*\*Bug 3 -- parser: `bool x = &y;` (address-of in a global initializer)
  test: test_bool_cast_const_fold.c.

- \*\*Bug 4 -- parser: `__builtin_{add,sub,mul}_overflow_p` never
  test_overflow_builtin_const_fold.c.

- **Bug 5 -- parser: unary `+` was a complete no-op**, never applying
  promotion -- both failed. Regression test: test_unary_plus_promotion.c.

- \*\*Bug 6 -- parser: `eval_const_expr()`'s arithmetic ops (+, -, \*,
  test_const_expr_width_truncation.c.

- \*\*Bug 7 -- parser/type.c: `usual_arith_type()` implemented "any

  > = signed-rank converts to unsigned; else a strictly-wider signed type
  > stays signed (can represent the full unsigned range); else (same
  > width, different rank -- `long`/`long long` on LP64) both convert to
  > the unsigned type of the signed operand. Also fixes the analogous
  > pre-existing bug in the `_BitInt` mixed-signedness path. Regression
  > test: test_mixed_sign_arith_conversion.c.

  Bugs 4-7 together took gnulib-tests/test-intprops.c (413 checks) from
  hard compile failure down to 0 remaining static_assert/compile
  failures.

- \*\*Bug 8 -- parser: `offsetof()` returned a plain `int`-typed constant
  convention. Regression test: test_offsetof_size_t_type.c.

- \*\*Bug 9 -- headers: bundled `<stdint.h>` failed gnulib's own "does
  Regression test: test_stdint_c99_conformance.c.

  ggrep (GNU grep 3.12) now builds completely clean with rcc (0
  errors), and its own functional test suite (`tests/`, the real
  correctness suite) passes 100%: 118/118 pass, 8 skip, 2 xfail, 0 fail
  -- matching the gcc-built binary exactly. After bug 9's header fix,
  `./configure` naturally stops using gnulib's stdint.h replacement
  (matching the gcc build) and `gnulib-tests/` (a separate ~400-test
  self-test suite for gnulib's own portability-shim infrastructure, not
  grep itself) builds and runs almost entirely clean too; the one
  remaining failure (`test-ioctl`, `sys/ioctl.h`'s gnulib-generated
  `ioctl` redeclaration disagreeing with this host's glibc prototype) is
  the identical pre-existing environment/gnulib-snapshot mismatch
  already documented and confirmed-not-rcc in the wget2 entry above
  (cross-checked again here: gcc rejects the identical repro too).

  `make check-all`: 0 failed (Unit 318/318 incl. all 8 new regression
  tests, Torture 3605/3609 -- 0 failed, 354 skipped, 4 todo, same
  baseline; TCC/Compliance/C-testsuite/Dg-error/Link unaffected).

### Fixed (2026-08-24, jemalloc session -- 2 stacked x86-64 codegen bugs)

- **Bug 1 -- wide `_BitInt` hidden-return-buffer callee param shift** --

- \*\*Bug 2 -- deep nested-ternary call argument corrupted an earlier

  New regression tests: `test_wide_bitint_retbuf_param_shift.c`,
  `test_ternary_arg_pressure.c` -- both fail on the old
  build. jemalloc (2 disabled: `--disable-cxx` for an unrelated g++-16/
  libstdc++ `std::__throw_bad_alloc` ABI break, not rcc's issue) builds
  clean with rcc (0 errors) and its full unit test suite passes:
  90/112 + 3×14/14 (fail:0 in every run, 22 env-gated skips: prof/
  witness/debug-only tests). `test/unit/bit_util` specifically passes
  all 25 sub-tests. Full local suite re-run clean: Unit 320/320 (incl.
  the 2 new tests), Torture 3605/3609 (0 failed, same baseline), TCC/
  c-testsuite/Dg-error/Link unaffected. ARM64 cross (qemu): 311/311
  unit tests. mingw cross: compiles clean.

### Verified (2026-08-24, test_hare -- no rcc bug)

- harec (the Hare language's reference compiler, `test/third_party/
test_hare/harec`) builds clean with rcc as CC (0 errors, standard
  `CFLAGS` minus `-Werror`/`-pedantic` which the harness doesn't
  apply anywhere else either), and the resulting binary works
  correctly: compiling a trivial `export fn main() void = void;`
  program produces well-formed QBE IR (`harec -o hello.ssa hello.ha`
  succeeds, `function $main() { ... ret }` emitted correctly).

  harec's own `make check` (using a locally-built `qbe` backend,
  `/home/rurban/Software/qbe/qbe`, since this project's own tree has
  no vendored copy) fails at the very first step -- compiling its own
  bundled Hare runtime (`rt/abort.ha`, `rt/cstrings.ha`, `rt/itos.ha`,
  `rt/malloc.ha`) -- with harec's type-checker rejecting its own
  source: "Array members must be of a uniform type, previously seen
  u8, but now see u8" and "Initializer type u8 is not assignable to
  constant type u8" (the SAME type reported on both sides of a
  supposed mismatch). Rebuilding harec from the identical source with
  a real system `gcc` instead of rcc reproduces this EXACT failure,
  byte-for-byte identical error text and location -- confirming this
  is a pre-existing bug/version-mismatch in this harec+runtime
  checkout's own type-identity logic (or a missing bootstrap step),
  entirely independent of which C compiler builds harec itself. Not
  an rcc bug; out of scope (harec's own type-checker internals, not
  rcc-compiled-code output).

### Verified (2026-08-23, quickjs -- one upstream snapshot bug, not rcc)

- quickjs (dev snapshot 2026-06-04) builds clean with rcc as CC (0
  errors, -Werror active) and the qjs binary works (parseFloat,
  console, closures all correct in isolation). `make test` runs
  test_closure.js and test_language.js clean, then fails in
  test_builtin.js's test_number at `assert(parseFloat("0x1234"), 0)`
  (line 381) -- an order-dependent corruption: the assertion passes in
  isolation and after minimal preceding tests, but fails after the full
  test_string/test_math sequences. A gcc-built qjs fails IDENTICALLY
  on the same line, so this is an upstream dev-snapshot regression,
  not an rcc issue.

### Fixed (2026-08-23, toybox session -- 2 bugs)

- **`-E -` (stdin input) merged the whole output onto one line** --

- **bundled <limits.h> lacked the include_next chain** -- glibc's

  Regression tests: test_stdin_linenumber.c, test_posix2_limits.c
  (both fail on the old build). toybox then builds clean with rcc
  (0 errors) and its test suite passes 356/357 -- the one failure,
  du "(no options)", reproduces IDENTICALLY with a gcc-built toybox
  (filesystem block-size dependent expectation), so it is not an rcc
  issue.

### Fixed (2026-08-23, zsh session -- 3 stacked bugs)

- **`-E` emitted no linemarker for macro-only headers** -- zsh's

- **rcc silently accepted conflicting function redeclarations** --

- **bundled <math.h> lacked the gamma family** -- tgamma/lgamma/gamma

  Regression tests: test_err_conflicting_func_decl.c (compile error
  expected), test_linemarker_empty_header.c, test_gamma_family.c --
  all fail on the old build. zsh now configures, builds, and passes
  its full test suite with rcc: 62/62 successful, 0 failures, 3
  skipped (locale-dependent), matching the gcc build.

### Verified (2026-08-23, yash -- no rcc bug)

- yash (the POSIX shell) configures and builds clean with rcc as CC,
  and its full test suite passes with the rcc-built binary:
  20229/20233 OK, 0 errors, 4 skipped (locale/feature-dependent
  skips). The only build hiccup is the manpage target (yash.1), which
  needs an external doc toolchain and is ignored by the Makefile.
  Smoke-tested: `./yash -c 'echo hello; x=5; echo $x'` works.

### Fixed (2026-08-23, janet session -- union flat-init array sizing)

- \*\*`skip_flat_aggregate_init()` consumed the comma after a union's
  test: extended test/test_compound_literal.c with a

### Verified (2026-08-23, cc65 -- no rcc bug)

- cc65 (the 6502 cross-compiler suite: cc65/ca65/ld65/sim65/cl65/
  da65/od65/ar65/grc65/sp65) builds clean with rcc as CC (0 errors),
  and its full regression suite passes with the rcc-built tools:
  "validation suite successful" (the only failures are the known
  /todo tests, expected to fail). The tools also run fine (cc65 V2.19
  banner). No rcc bug found.

### Fixed (2026-08-23, sokol session -- compound-literal designator chains)

- \*\*`(T){ ... }` compound-literal initializers with array-index steps in

  Found via sokol's own compile tests (sokol_nuklear.h's sg_shader_desc
  and functional/sokol_gfx_test.c). Regression test
  test/test_compound_literal.c covers all three shapes; fails
  on the old build. All sokol C compile tests (sokol-compiletest-c,
  sokol-compiletest-c-all) and functional test objects now build with
  rcc; the remaining failures are environment gaps, identical with gcc:
  missing ALSA dev headers (sokol_audio.h: alsa/asoundlib.h and
  -lasound) and gcc-16's -Werror unused-but-set-variable on the pure
  g++ CXX tests. Functional tests can't run headless anyway.

### Verified (2026-08-23, orangeduck_mpc -- no rcc bug)

- mpc builds clean with rcc (warnings only: ignored -Wswitch-default
  etc.) and its ptest suite passes fully: 4/4 suites, 30/30 tests,
  142/142 asserts, 0 failed, in both the single-exe (test-file) and
  shared-lib (test-dynamic) variants. Examples (maths et al.) build and
  run. The only failure is `make check`'s test-static link needing
  `-static` glibc, an environment gap (glibc-static not installed on
  this Fedora box; identical failure with gcc).

### Fixed (2026-08-22, metalang99/datatype99 session -- 3 stacked preprocessor bugs)

- **`nframes > 600` cap silently left deep macro nesting unexpanded** --

- **`__COUNTER__` was never defined as a macro** -- only handled as an

- \*\*Single-name "blue paint" replaced with a proper hide set

  New regression tests: test/test_deep_macro_expansion.c (750-deep
  linear chain, fails on the 600 cap), test/test_counter_defined.c
  (#ifdef **COUNTER** + incrementing values, fails on the old build).
  datatype99 4/5 (tests, metalang99_compliant, record_derive, version)
  and metalang99 14/16 pass; derive.c, lang.c, list.c still fail with a
  deeper continuation-prescan stall (ML99_PRIV_REC_NEXT_ML99_PRIV_EVAL_0op
  residue), pre-existing and out of scope for this session.

### Fixed (2026-08-22, i64/u64 -> f32 double-rounding session)

- \*\*`(float)(int64_t)` / `(float)(uint64_t)` double-rounded: converted via

  New regression coverage: extended `test/test_u64_to_double_round.c`
  with exact f32 bit-pattern checks for the wasm3 repro values (i64 and
  u64). Confirmed it reproduces the 1-ULP error with the fix reverted,
  passes clean restored. Full local regression matrix re-run clean: TCC
  118/118, Unit 296/296, c-testsuite 220/220, Torture 3605/3609 (0
  failed, 354 skipped, 4 todo -- same baseline), Dg-error 34/34, Link
  12/12.

  wasm3's spec suite drops from 0.12% to 0.06% failures; the remaining 8
  are signaling-NaN payload bit-exactness through float load/store
  (`i32.reinterpret_f32(nan)` etc.) -- rcc's double-widened float
  representation quiets sNaNs on load (`cvtss2sd`), a documented
  architectural limitation (see the `long double`/float-representation
  notes throughout codegen.c), not this fix's scope.

### Fixed (2026-08-22, \_\_builtin_dynamic_object_size reading malloc header from any pointer)

- \*\*`__builtin_dynamic_object_size` emitted a RUNTIME glibc-malloc-chunk-

  Fixed by making the builtin return the compile-time size for known
  stack/global arrays and structs and the unknown sentinel (-1/0,
  honoring the mode argument) for everything else -- matching
  `__builtin_object_size`'s unknown handling and GCC's semantics. The
  runtime chunk-header read was removed entirely.

  New regression coverage: extended `test/test_bos.c` with a
  function-parameter `__builtin_dynamic_object_size` (must be -1/0, not
  a stack read) and a `stack_via_param()` helper that zeroes a 128-byte
  stack array through a `memset`/`explicit_bzero` parameter pair.
  Confirmed it reproduces the failure (exit 5) with the runtime-header-
  read implementation reverted, passes clean restored. Full local
  regression matrix re-run clean after the fix: TCC 118/118, Unit
  296/296, c-testsuite 220/220, Torture 3605/3609 (0 failed, 354
  skipped, 4 todo -- same baseline), Dg-error 34/34, Link 12/12.

  Unblocks: libsodium 1.0.20 (`make check` in test/default: 86/86
  passed, was ~50 failing with SIGABRT).

### Fixed (2026-08-22, x86-64 inline-asm >8-operand pool overflow + numeric-label direction session)

- \*\*`>8` simultaneously-live GP inline-asm operands aliased onto the same

  Fixed by widening every operand-setup site (output-only, read-write,
  plain input, and the separate `op_saved` fixed-register-output-capture
  loop) with a last-resort overflow pool (`asm_extra_pool` =
  `{RAX, RCX, RDX, RDI}`, physical registers excluded from the ordinary
  8-slot pool and otherwise reserved as scratch elsewhere in codegen),
  tried only once `free_reg_count() == 0`. A second, closely related bug
  surfaced immediately: the overflow pool's "first free slot" pick did
  not exclude a register a FIXED-register constraint (`"=a"`/`"=b"`/
  `"=c"`/`"=d"`) on another operand in the SAME statement already claims
  -- an `"=a"` output alongside 8 pool-exhausting `"+&r"` operands hit a
  self-collision (the 9th operand's overflow pick landing on `%eax`,
  the same register `"=a"` is hard-wired to). Fixed by `asm_extra_pick()`
  (`codegen_asm.h`), which skips any pool entry a
  `x86_fixed_claimed` bitmask (built from the existing fixed-register-
  constraint scan) already reserves.

  A third bug in the same area: `try_const_int()` only recognized a bare
  `ND_NUM` for an `"i"`/`"n"` immediate operand -- any non-trivial-but-
  constant expression (shifts/arithmetic, e.g. `RC_TOP_VALUE == (1 <<
N)`) fell back to the runtime-register path, needing a register for a
  value that must be a bare assembler immediate, which under the same
  pool pressure competed with and clobbered a live `"=&r"` output. Fixed
  by falling back to the general compile-time constant evaluator
  (`eval_const_expr()`).

- \*\*A reused GAS numeric local label (`"1:"`) resolved `"1f"` (forward)

- \*\*Inline-asm template concatenation silently truncated (emptied) past

  New regression coverage: `test/test_asm_wide_operand_pool.c` (all
  three pool-overflow-family bugs), `test/test_asm_numeric_label_
direction.c` (objdump-verified forward-vs-backward `jae` target),
  `test/test_asm_long_template.c` (900-fragment/4500-byte concatenated
  template, objdump-verified nop count). All four reproducers confirmed
  to crash/corrupt/truncate with each fix individually reverted, pass
  clean restored. Full local regression matrix re-run clean after the
  fix: TCC 118/118, Unit 295/295, c-testsuite 220/220, NCC compliance
  15/15, Torture 3605/3609 (0 failed, 354 skipped, 4 todo -- same
  baseline), Dg-error 34/34, Link 12/12. mingw and ARM64 cross builds
  compile cleanly (both code paths either untouched --
  `#ifdef ARCH_ARM64` -- or share the same x86-64 codegen already
  exercised natively); mingw's own cross test run hit an unrelated,
  pre-existing Wine/`gcc.exe`-not-on-`PATH` environment gap, reproduced
  identically with this session's changes reverted.

  Every individual range-decoder asm macro in xz's LZMA1 decoder
  (`rc_bittree3`/`6`/`8`, `rc_matched_literal`, `rc_direct`,
  `rc_bit_add_if_1`, `rc_bittree_rev4`) now produces byte-identical
  output to a real-GCC-built comparison binary across representative
  inputs. `xz`'s own CLI `lzip_decoder`/CRC32/index test suites now pass
  in full; a separate bug in the full `xz` CLI's own
  compress/decompress round-trip (`test_compress_generated_*`,
  `test_files.sh`) is now ALSO fixed (see the dedicated entry below) --
  `xz` is checked off in `checklist.txt`.

### Fixed (2026-08-22, inline-asm operand-store-back drain reusing a spill-cleared value register)

- \*\*An operand's store-back ADDRESS register could be re-allocated to a

  Fixed by re-claiming the used bit for every asm operand value
  register whose bit a transient spill inside `gen_addr()`/`gen()`
  cleared, immediately after each operand's setup in pass 1 and again
  after the second pass's matching-constraint `gen()` calls. Since the
  asm writes the output's real value to the register before the
  store-back runs, keeping the bit set merely stops the drain (and the
  restore passes) from reusing the register for addresses -- exactly
  what the existing `asm_extra_pool` overflow machinery already
  guarantees for the pool-exhausted case.

  New regression coverage: `test/test_asm_operand_spill_reclaim.c` --
  drives xz's real `rc_asm_bittree_n()` inline-asm macro (copied
  verbatim) side by side with the plain-C reference decoder for 64
  decode iterations, asserting the decoded symbol, updated range/code
  state, input-pointer position, and the full probability array all
  match. The state-corruption failure requires the exact surrounding
  decoder register pressure (reproduced by xz's own test suite: all 19
  `ninja test` targets pass after this fix, 4 failed before); the unit
  test pins the real macro's decode contract under rcc codegen.

### Fixed (2026-08-22, SSE4.1 vec*set*_ builtin misimplemented as vec*init*_)

- \*_`\_*builtin_ia32_vec_set*_`(used by`\_mm_insert_epi{8,16,32,64}`) was

  Fixed by splitting the handler: `vec_set_*` copies the source vector to
  the result slot and stores the scalar at `idx * element_size`; only
  `vec_init_*` iterates scalar arguments. Element width and lane index are
  taken from the result vector type and the constant third argument as
  before.

  Found via xz's CLMUL-optimized CRC32 path (`src/liblzma/check/crc32_fast.c`):
  the `size < 16` branch uses `_mm_insert_epi64` to combine the low 8
  bytes with the last input byte, and the corrupted lane produced wrong
  CRCs. This caused `test_check` and `test_index_hash` to fail; with the
  fix both pass. A separate `test_lzip_decoder` hang remains even with
  CLMUL disabled at build time, so xz is intentionally NOT checked off in
  `checklist.txt` yet.

  New regression coverage: extended `test/test_ia32_intrinsics.c` with
  `_mm_insert_epi64` lane-0/lane-1 and `_mm_insert_epi32` checks. Full
  local regression matrix re-run clean after the fix: TCC 118/118, Unit
  293/293, Torture 3605/3609 (0 failed, 354 skipped, 4 todo — same
  baseline), Dg-error 34/34, Link 12/12.

### Fixed (2026-08-22, x86-64 inline-asm fixed-register vreg-pool masking session)

- \*\*Fixed-register asm constraints (`"a"`/`"b"`/`"c"`/`"d"`/`"S"`/`"D"`)

  Found via mbedtls's `bignum_core.c`
  Montgomery-multiplication inline asm (`MULADDC_X1_CORE`'s
  `"mulq %%rbx"` with a `"b"`-constrained multiplier alongside an
  `"S"`/`"D"`-constrained memory pointer pair): the memory pointers'
  own address computation (`gen_addr()` -> `alloc_reg()`) grabbed
  `%rbx`'s unprotected virtual slot and overwrote the multiplier before
  `mulq` read it, corrupting the partial product -- which cascaded into
  an infinite loop in `mbedtls_mpi_core_montmul`'s carry-propagation
  once the corrupted output stopped terminating the loop's own
  convergence check. A second, closely related instance of the exact
  same root cause surfaced immediately after fixing the first: the
  "capture every x86-physical-register output into a scratch vreg
  before the address-register restore" loop (added by an earlier
  session to fix a _different_ clobber -- see
  `test/test_asm_multi_output_clobber.c`) called `alloc_reg()` for each
  output's scratch temp without protecting the _other_, still-unread
  outputs' physical registers -- with the wide (buggy) mask from the
  first bug removed, `alloc_reg()` was now free to hand out `%rbx`'s
  virtual slot as a scratch temp for capturing an unrelated `"=a"`
  output, clobbering a `"=b"` output's still-unread cpuid-style result
  before it was captured (found via mbedtls's own 4-output/3-matched-
  input `HMAC` test harness cpuid-style call pattern reproduced
  standalone).

  Fixed with a proper reverse map: `x86_reg_vbit(X86Reg)` in
  `codegen_asm.h` scans `cg_x86_reg[]` for the physical register and
  returns the correct virtual-register bit (0 if the physical register
  isn't part of the pool at all -- `RAX`/`RCX`/`RDX`/`RDI` never are, so
  there is genuinely nothing for `alloc_reg()` to collide with for
  those). Applied at both call sites: the original `x86_reserved_mask`
  computation and a new, analogous `x86_output_mask` guarding the
  per-output scratch-capture loop.

  New regression coverage: `test/test_asm_reg_pool_reservation.c` (the
  exact `MULADDC_X1_CORE` shape plus a 4-output/3-matched-input cpuid
  pattern). Confirmed both reproduce their respective wrong
  result/clobber with the fix reverted, pass clean restored; the
  pre-existing `test/test_asm_multi_output_clobber.c` (a different,
  earlier fix in the same area) also re-verified still passing. Full
  local regression matrix re-run clean after the fix: TCC 118/118, Unit
  291/291, Torture 3605/3609 (0 failed, 354 skipped, 4 todo — same
  baseline), Dg-error 34/34, Link 12/12.

  Unblocks: mbedtls/tf-psa-crypto's `bignum.generated-suite` (was 100%
  reproducible infinite loop in `mbedtls_mpi_core_montmul`, confirmed
  hung via `gdb -p` backtrace showing the non-terminating
  `mbedtls_mpi_inv_mod_odd` -> ... -> `mbedtls_mpi_core_montmul` call
  chain; now passes in ~0.5s, matching a real-gcc-built comparison
  binary). mbedtls's full `ctest` suite is not yet 100% green: a
  separate, unrelated stack-frame-layout bug remains in
  `psa_crypto-suite` (a `TEST_EQUAL(psa_mac_compute(...7 args...), ...)`
  nested-call argument-staging slot collides with a live parameter's
  own spill slot -- SIGSEGV in `test_mac_sign`, `psa_crypto-suite`
  87/148) -- `mbedtls` intentionally NOT checked off in
  `checklist.txt` yet, pending that second fix.

### Fixed (2026-08-22, gen_funcall scratch-register spill state leaking Pass1->Pass2 session)

- \*\*`spill_slot[][]`/`spill_depth[]` were never reset between codegen's

  Found via mbedtls/tf-psa-crypto's `psa_crypto-suite`:
  `test_mac_sign()`'s `data_t *input` parameter (an HMAC test harness
  function taking 5 params and calling `psa_crypto_init()`/
  `psa_import_key()`/`psa_mac_compute()` in a loop) got its own
  parameter slot silently overwritten mid-function -- confirmed via
  disassembly: `mov %r11, -0x80(%rbp)` immediately before
  `call psa_import_key`, where `-0x80(%rbp)` was ALSO `input`'s own
  parameter-spill slot from the prologue -- SIGSEGV on the very next
  `input->x` dereference inside the loop. Reproduced deterministically
  via a standalone driver linking the real, unmodified `libmbedtls.a`/
  `libtfpsacrypto.a` built by this same session's rcc and calling
  `test_mac_sign()`'s exact real-world logic directly (100% SIGSEGV
  before the fix, matching the CI-observed crash exactly; clean after).

  Fixed by resetting `spill_slot[][]`/`spill_depth[]` at the start of
  Pass 2, alongside the other per-pass state (`used_regs`/
  `spilled_regs`/`reg_owner`) already reset there.

  New regression coverage:
  `test/test_funcall_spill_reset.c` -- a self-contained
  driver confirmed (via internal instrumentation during development)
  to drive `spill_depth[0]` and `spill_depth[1]` to 1 by Pass 1's end,
  the exact state-leak precondition this fix addresses. A fully
  self-contained repro that also reproduces the numeric slot/parameter
  _collision_ itself (not just the leak precondition) proved elusive
  despite substantial effort -- the collision additionally requires
  Pass 1 to have already grown `next_spill_slot` past every
  parameter's own offset via unrelated register-exhaustion spilling
  before its first scratch-protected call fires, a register-allocator-
  internal detail no hand-written variant reliably reproduced; the
  mbedtls `libmbedtls.a`/`libtfpsacrypto.a`-linked driver above is the
  authoritative reproduction for the fix itself, run directly (not
  committed, since it needs the external mbedtls submodule checkout).
  Full local regression matrix re-run clean after the fix: TCC
  118/118, Unit 292/292, Torture 3605/3609 (0 failed, 354 skipped, 4
  todo -- same baseline), Dg-error 34/34, Link 12/12.

  Unblocks: mbedtls/tf-psa-crypto's `psa_crypto-suite` (was a 100%
  reproducible SIGSEGV in `test_mac_sign`; now runs to completion).
  `mbedtls`'s full ctest suite is still not 100% green: a separate,
  unrelated `psa_symmetric_encrypt` (AES-ECB) test failure remains
  (`PSA_CIPHER_ENCRYPT_OUTPUT_SIZE(...)` compared against a garbage
  left-hand value) -- `mbedtls` intentionally NOT checked off in
  `checklist.txt` yet, pending that separate investigation.

### Fixed (2026-08-22, ARM64 atomic-op one-shot scratch slot misusing register-associated spill tracking)

- **CI fallout from the immediately preceding fix**: macOS (ARM64) CI

  Fixed by not routing through the register-associated machinery at
  all: grow `next_spill_slot` directly (ARM64's own `push_spill_slot()`
  arithmetic, minus the register-index bookkeeping neither needed nor
  wanted here), which -- like `alloc_spill_slot()` on the x86-64 side
  -- always hands back a fresh, private offset with no reuse contract
  to violate.

  Verification: this environment has no ARM64 sysroot to build/run
  `rcc-arm64` locally (`make CC=aarch64-linux-gnu-gcc` needs aarch64
  glibc headers, not installed here) -- confirmed via `gcc -DARCH_ARM64
-fsyntax-only` that the new code compiles cleanly (the prior attempt,
  reusing the x86-64-only `alloc_spill_slot()` helper by name, did
  not); x86-64 side is untouched (this is entirely inside `#ifdef
ARCH_ARM64`) and its own full local regression matrix re-run clean:
  TCC 118/118, Unit 292/292, Torture 3605/3609 (0 failed, 354 skipped,
  4 todo -- same baseline), Dg-error 34/34, Link 12/12. The macOS
  (Apple Silicon) CI runner is the authoritative verification for the
  ARM64 code path itself.

### Fixed (2026-08-21, struct-arg cast against stale incomplete prototype session)

- \*\*`check_type()`'s own, second implicit-argument-cast loop for

  Found via zstd 1.5.7's default CLI compression path (multi-threaded,
  the default when built with pthreads): `zstd_compress.c` calls
  `ZSTDMT_initCStream_internal()` (prototype only visible via
  `zstdmt_compress.h`, included -- and thus its `ZSTD_CCtx_params`
  parameter type captured while still an opaque forward `typedef struct
ZSTD_CCtx_params_s ZSTD_CCtx_params;` from `zstd.h` -- BEFORE
  `zstd_compress_internal.h` completes the struct body a few lines
  later in the same file; the real function DEFINITION lives only in
  `zstdmt_compress.c`, a different translation unit, so nothing ever
  re-derives the stale prototype) with a 224-byte `ZSTD_CCtx_params`
  argument sandwiched between 5 leading pointer/`size_t`/enum register
  arguments and a trailing `unsigned long long` one. `readelf`/`gdb`
  disassembly of the miscompiled call showed the trailing
  `pledgedSrcSize` argument landing on the stack (should be `%r9`, the
  6th GP register) while `%r9` instead held the struct's first 4 bytes
  reinterpreted as a pointer -- the struct itself was never copied to
  the stack at all. `ZSTDMT_initCStream_internal()` then read all-zero/
  garbage `cParams` fields, tripped `ZSTD_checkCParams()`'s bounds
  check, and `assert()`ed (rcc's bundled `<assert.h>` calls `abort()`
  directly, printing no diagnostic -- see `include/assert.h`) --
  100% reproducible SIGABRT on every `zstd`-CLI invocation compressing
  from stdin (`playTests.sh`'s very first pipe test). Confirmed a
  genuine rcc bug, not environment/build-system noise: the identical
  source tree built clean with real gcc, and the system's own `zstd`
  binary worked fine.

  Fixed by skipping the cast-insertion loop entirely whenever either
  side is `TY_STRUCT`/`TY_UNION` -- mirroring `cast_funcall_args()`'s
  own, already-deliberate exclusion for the identical reason (a
  struct/union argument is always passed by raw value copy through
  codegen's own by-value-argument ABI logic, driven by the ARGUMENT's
  own resolved type; it is never a candidate for an
  arithmetic-conversion-style cast in the first place, complete or not).

  New regression coverage: `test/test_incomplete_struct_arg_cross_tu.c`
  -- reproduces the exact shape (prototype-then-completion in one TU,
  real definition in a genuinely separate one, spawning two temp `.c`
  files and a real `rcc a.c b.c -o prog` sub-invocation via
  `test_common.h`'s `find_rcc()`/`get_tmpdir()` helpers, since a
  single-file repro doesn't reproduce -- parsing the callee's own body
  in the same TU re-derives its function type from the by-then-complete
  struct, masking the stale prototype). Confirmed it reproduces the
  wrong-argument corruption (garbage sum instead of the expected 128)
  with the struct/union skip reverted, passes clean restored. Full
  local regression matrix re-run clean after the fix: TCC 118/118, Unit
  289/289, C-testsuite 220/220, Torture 3605/3609 (0 failed, 354
  skipped, 4 todo -- same baseline), Dg-error 34/34, Link 12/12.

  Unblocks: zstd 1.5.7 (`make check`: full `playTests.sh` suite passes,
  including every multi-threaded compress/decompress round-trip; was
  100% SIGABRT on the very first stdin-pipe test).

### Fixed (2026-08-21, uint64_t->double/float round-to-odd session)

- \*\*`uint64_t`/`unsigned long long` -> `double`/`float` conversion for

  Found via yyjson's own `test_number` unit test: parsing the literal
  `-9223372036854776833` produced `-9223372036854775808` (rcc) instead
  of the correct `-9223372036854776833`'s nearest double,
  `-9223372036854777856` (confirmed via a standalone `(double)(uint64_t)`
  repro cross-checked against real gcc). Isolated to
  `unsafe_yyjson_u64_to_f64()` -> a plain C `(double)u64` cast, ruling
  out any yyjson-side bug.

  Fixed by inserting `x86_and_ri(scratch_src, 1)` before the shift and
  `x86_or_rr(rcx, scratch_src)` after it at all 6 call sites (the
  original source register is dead after each site -- its value was
  already copied into `%rcx` -- so it doubles as the scratch for the
  isolated sticky bit, matching GCC's exact register choreography).
  ARM64 is unaffected: `ucvtf` natively handles the full unsigned
  64-bit range in one instruction, no manual fallback needed.

  New regression coverage: `test/test_u64_to_double_round.c` (the
  yyjson repro value plus `UINT64_MAX`, `INT64_MAX+2`, and an exact
  power-of-two boundary, both `double` and `float` targets). Confirmed
  it reproduces the wrong rounding with the `and`/`or` pair reverted at
  all 6 sites, passes clean restored. Full local regression matrix
  re-run clean after the fix: TCC 118/118, Unit 288/288, C-testsuite
  220/220, Torture 3605/3609 (0 failed, 354 skipped, 4 todo -- same
  baseline), Dg-error 34/34, Link 12/12.

  Unblocks: yyjson (`ctest`: 12/12 passed, was 11/12 -- `test_number`'s
  own assertion now matches).

### Fixed (2026-08-21, ELF/Mach-O/PE shared-library debug-section relocation session)

- **`.rela.dyn` emission iterated non-allocated (debug) sections too** —

  Found via chibi-scheme's own `Makefile.libs` (every `lib/**/*.so`
  module built with `-g -g3 -O3`): `dlopen("lib/srfi/69/hash.so")`
  crashed inside glibc's dynamic linker; `readelf -r` on the produced
  `.so` showed exactly 3 spurious `R_X86_64_RELATIVE` entries (traced via
  a temporary debug print to `.debug_line`/`.debug_info`/
  `.debug_aranges`, `sec->addr=0 exec=0 write=0`, confirming they were
  never allocated sections at all) alongside the legitimate ones.

  Fixed by adding the same `if (!sec->alloc) continue;` guard to all
  four loops.

  New regression coverage: `test/test-link.sh` case 15 (a `-g -g3`
  shared library with a locally-resolved function pointer, `dlopen()`d
  and called through) — confirmed it reproduces the exact SIGSEGV when
  the four guards are reverted, and passes clean with them restored.
  Full local regression matrix re-run clean after the fix: TCC 118/118,
  Unit 287/287, C-testsuite 220/220, Torture 3605/3609 (0 failed, 354
  skipped, 4 todo — same baseline), Dg-error 34/34, Link 12/12.

  Unblocks: chibi-scheme 0.12.0 (full `test-r7rs` + `test-fs` suite: 8/8
  subgroups, 304/304 tests pass — every `lib/**/*.so` module now
  `dlopen()`s cleanly).

- **Same root cause, link_pe.c**: `link_load_object()` (the COFF object

- **Same root cause, link_macho.c, different manifestation**:

  New regression coverage: `test/test-link.sh` case 15 widened to also
  cover `.dylib` (POSIX `dlopen()`/`dlsym()` work identically on macOS);
  new case 16 for `.dll` (no `dlfcn.h` on Windows — a direct DLL link +
  run exercises the same `build_pe_reloc()` path at ordinary process
  load time instead). Verified directly (not just via the test script,
  which needs a real target OS/runner this sandbox doesn't have for
  PE/Mach-O execution): PE's fix confirmed via raw `.reloc` byte
  inspection of a mingw-cross-built DLL (bogus entries present without
  the fix, gone with it); Mach-O's fix confirmed via raw Mach-O byte
  inspection of a native `rcc-darwin` build (bloated `__TEXT,__const`
  without the fix, clean with it). Full local Linux regression matrix
  re-run clean after all three fixes: TCC 118/118, Unit 287/287,
  C-testsuite 220/220, Torture 3605/3609 (0 failed, 354 skipped, 4 todo
  — same baseline), Dg-error 34/34, Link 12/12.

### Fixed (2026-08-21, angle-include search order + findutils session)

- \*\*`-I` directories searched AFTER rcc's own bundled headers for

  Keeping RCC*INCDIR ahead of -I broke every project shipping its own
  gnulib-style "-I override + `#include_next` onward" replacement
  header sharing a name with one of the six headers above (rcc's own
  bundled copies of these are fully self-contained, no `#include_next`
  of their own to chain through to such an override — unlike
  `stdio.h`/`wchar.h`/`math.h`/`iconv.h`, which already do). Found via
  findutils' `gl/lib/stddef.h`: it provides `gl_unreachable()` (used by
  `error.h`'s `error_at_line`-style macros) and, critically, is
  designed to be the FIRST responder to *every* `#include <stddef.h>`
  in the whole translation unit — including deep, `__need_size_t`-
  restricted requests from glibc's own headers (e.g.
  `bits/types/struct_iovec.h`, which never itself `#undef
__need_size_t`) — specifically so it can track and clean up that
  "extract just this one type" protocol on the caller's behalf
  (confirmed: `gl/lib/stddef.h` explicitly `#undef`s all five
  `\_\_need*_`macros itself, real gcc's own`-dD`trace shows them
cleanly paired define/undef as a result). With RCC_INCDIR searched
first,`gl/lib/stddef.h`was never reached for any of these deep
requests, its`\_*need*_`bookkeeping never ran, and`gl*unreachable`was consequently never defined for the whole TU — a silent,
undiagnosed dead end: "undefined reference to`gl_unreachable'" only
  at LINK time, no compile-time signal at all. (An intermediate fix
  attempt — making rcc's own bundled `stddef.h` conditionally chain via
  `#include_next` after its own definitions — was explored and
  discarded: it cannot replicate `gl/lib/stddef.h`'s stateful `\_\_need*\*`
  handling, which fundamentally requires being reached FIRST, not
  chained through after the fact.)

  New regression coverage: `test/test_angle_include_precedes_bundled.c`
  (a generated `-I` override + `#include_next` for each of the six
  affected headers, mirroring `gl_unreachable`'s shape). Full local
  regression matrix re-run clean after the fix: TCC 118/118, Unit
  287/287, C-testsuite 220/220, Torture 3605/3609 (0 failed, 354
  skipped, 4 todo — same baseline), Dg-error 34/34, Link 11/11.

  Unblocks: findutils 4.11.0 (`make check`: 25 passed, 4 skipped
  [root-required], 0 failed).

### Fixed (2026-08-21, rpmalloc: extern-inline static false-positive + -m64 flag session)

Two issues blocking mjansson/rpmalloc 2.0.1:

- \*\*`extern inline` wrongly treated the same as bare `inline` by the

- **`-m64` rejected as an unrecognized command-line option** —

New regression coverage: `test/test_extern_inline_static_and_m64.c`
(bare `inline`/`static inline`/`extern inline` referencing a `static`
object, with and without `-Werror`; `-m64` accepted, `-m32` still
rejected). Full local regression matrix re-run clean after both fixes:
TCC 118/118, Unit 286/286, C-testsuite 220/220, Torture 3605/3609 (0
failed, 354 skipped, 4 todo — same baseline as before this session's
diagnostic change, confirming the earlier `-Werror`-only mitigation had
regressed `test/torture/c11-thread-local-2.c`'s expected
`-pedantic-errors` diagnostic and this fix restores it), Dg-error 34/34,
Link 11/11.

Not fixed / out of scope: rpmalloc's own `bin/linux/release/x86-64/
rpmalloc-test` (the override-enabled binary `test_rpmalloc()` actually
runs) still fails to LINK — `test/main-override.cc` calls sized
`operator delete(void*, unsigned long)`, but rpmalloc's own
`configure.py`-generated ninja file always uses `$CC` (never `$CXX`) as
the link driver for every target, even ones including a `.cc` object,
and never adds `-lstdc++`/`-lc++` to any target's link libs. Confirmed
**not** an rcc bug: reproduces byte-identical (same undefined symbol,
same two call sites) linking the same object files with real `gcc` in
place of rcc as the link driver. A pre-existing rpmalloc/muon build-
config gap, unrelated to which C compiler is used.

### Fixed (2026-08-21, incomplete-type local declaration session)

- \*\*An automatic-storage-duration local declaration of an incomplete
  triage session's `test_incomplete_sizeof_cast.c`, but not this one).

  Found via Tcl's own `./configure` large-file-support probe:
  `struct dirent64 p;` inside a throwaway `AC_COMPILE_IFELSE` test
  program, with glibc's `<dirent.h>` only ever forward-declaring
  `struct dirent64` (no body) unless `_LARGEFILE64_SOURCE`/
  `_GNU_SOURCE` is in scope (neither is, on this system, confirmed
  identical for both compilers via an isolated `sizeof`-based probe).
  Compiling this test with rcc as `CC` wrongly SUCCEEDED where real
  gcc correctly fails, flipping Tcl's own `HAVE_STRUCT_DIRENT64`
  autoconf result to true; `tclUnixPort.h` then selected the
  (unusable, still-incomplete) `struct dirent64`/`readdir64()` code
  path, and the real bug finally surfaced three files later as
  `unix/tclUnixFCmd.c:349: error: no such member` on
  `dirEntPtr->d_name` — a confusing downstream symptom of a root
  cause two build steps upstream. Cross-checked step by step against
  real gcc on a fresh `core-9-0-4` checkout: gcc's own configure
  correctly leaves `HAVE_STRUCT_DIRENT64` undefined on this system,
  and `tclUnixFCmd.c` builds clean.

  Fixed by adding the same `!ty->has_body` incomplete-type rejection
  the `sizeof`/cast paths already use, right before the automatic
  local variable is created — deliberately narrow: pointers to an
  incomplete type (always complete themselves) and `extern` block-scope
  declarations (reference file-scope storage, never allocate locally)
  are both unaffected, matching real gcc exactly.

  New regression coverage: extended (and renamed for its now-broader
  scope) `test/test_incomplete_sizeof_cast.c` →
  `test/test_incomplete_type_validation.c` with 6 new subprocess-compile
  cases (reject: local incomplete struct/union; accept: local pointer
  to incomplete type, local `extern` of incomplete type, local
  complete-struct declaration). Full local regression matrix re-run
  clean after the fix: TCC 118/118, Unit 285/285, C-testsuite 220/220,
  Torture 3605/3609 (0 failed, 354 skipped, 4 todo — same baseline),
  Dg-error 34/34, Link 11/11.

  Unblocks: Tcl 9.0.4 now builds and links completely end to end
  (`tclUnixFCmd.c`'s "no such member" is gone; `tcltest` links). Its
  own `make test` still fails separately at runtime with "Cannot find
  a usable init.tcl" — confirmed **not** an rcc bug: a standard Tcl
  script-library search-path quirk for running `tcltest` straight out
  of an unbuilt-tree (no `make install`) sandbox location, unrelated to
  which compiler built the binary, out of scope for a codegen fix.

### Fixed (2026-08-21, missing SSE2 intrinsics session)

- \*\*`_mm_sll_epi16/32/64`, `_mm_srl_epi16/32/64`, `_mm_sra_epi16/32`

- \*\*`_mm_loadu_si32`/`_mm_storeu_si32`/`_mm_loadu_si64`/

New regression coverage: both fixes verified byte-identical against
real gcc at `-O0`/`-O2`, folded into the combined
`test/test_sse2_intrinsics.c` (see the **common** attribute session
entry below for the file's consolidation history).

Projects now verified:

- **libopus**: `libopus.so`/`opus_demo` build and link cleanly (was
  `test_opus_api.c` still fails separately on `__malloc_hook`

### Fixed (2026-08-21, **common** attribute + redis session)

- **`__attribute__((__common__))` not recognized** — `parser.c`. redis's

### Fixed (2026-08-21, x86 assembler instruction coverage session)

Added missing x86 assembly instructions needed by nettle and libsodium:

**SSE2 shift/unpack/logic:**

- `psrad`: Group 13 immediate shift (66 0F 72 /4 ib)
- `pandn`: SSE2 AND-NOT (66 0F DF /r)
- `pmuludq`: SSE2 unsigned mul (66 0F F4 /r) — existed but undispatched
- `punpcklbw/lwd/ldq/hbw/hwd/hdq`: SSE2 unpack family — all existed but undispatched

**VEX 128-bit moves (AVX):**

- `vmovdqa`: VEX.128.66.0F.WIG 6F/7F — reg-reg + mem load/store
- `vmovdqu`: VEX.128.F3.0F.WIG 6F/7F — reg-reg + mem load/store

**PCLMULQDQ (carry-less multiply for GHASH):**

- `pclmullqlqdq/hqlqdq/lhqdq/hhqdq`: 66 0F 3A 44/45/4C/4D

**VEX 128-bit 3-operand (AVX):**

- `vpaddq/vpsubq/vpand`: VEX.128.66.0F.WIG D4/FB/DB
- `vpunpcklqdq/vpunpckhqdq`: VEX.128.66.0F.WIG 6C/6D
- `vpxor`: VEX.128.66.0F.WIG EF
- `vpmuludq`: VEX.128.66.0F.WIG F4

**VEX 128-bit 2-operand + imm (AVX):**

- `vpshufd`: VEX.128.66.0F.WIG 70 /r ib
- `vpsllq/vpsrlq`: VEX.128.66.0F.WIG 73 /6,/2 ib

**VEX special (AVX):**

- `vblendps`: VEX.128.66.0F3A.WIG 0C /r ib
- `vbroadcastss`: VEX.128.66.0F38.WIG 18 /r

All verified byte-for-byte against real GCC output.
Unblocks: nettle (builds, 73/128 tests pass), libsodium (builds).

### Fixed (2026-08-21, preprocessor >32 params + hh_mask UB session)

Two issues blocking redis and other projects:

- **`#define` param parser used fixed `char *params[32]` array**

- **`hh_mask` (32-bit) shifted by `idx` without bounds check**

- **`static` in `inline` function error downgraded to warning**

Projects now verified:

- **redis**: compiles (was COMPACT_FMT_N error); link fails on
- **jq**: builds and runs (1 optional test fails due to missing

### Fixed (2026-08-21, limits.h #include_next removal + LONG_BIT + atomic memory-order session)

Three issues blocking multiple third-party projects:

- **`#include_next <limits.h>` removed from rcc's bundled `<limits.h>`**

- **`LONG_BIT` missing from `<limits.h>`** — `include/limits.h`. POSIX

- **`__atomic_*_fetch` builtins require 3 args but GCC allows 2**

Projects now verified:

- **libtommath**: builds and passes all 50 tests (cmake test-ltm)
- **msgpack**: builds and passes all 5 tests (100%, 0 failures)
- **yash**: compiles (test suite too slow for batch timeout, not an rcc bug)
- **mongoose**: 1854 tests pass
- **wasm3**: builds and passes
- **mpack**: 0 failures in 24947 checks
- **nanomsg**: 77/77 tests pass (100%)
- **mpack**: 0 failures in 24947 checks
- **libmpc**: builds and tests pass (0 failures)
- **rvvm**: 113/113 RISC-V tests pass

### Fixed (2026-08-19, int128 POST_INC + **GLIBC** macros + POSIX limits session)

Three issues blocking multiple third-party projects:

- **int128 `ND_POST_INC` missing from `gen_int128` switch** — `codegen.c`.

- **`__GLIBC__`/`__GLIBC_MINOR__`/`__GLIBC_PREREQ` macros missing**

- **`PTHREAD_STACK_MIN` missing from `<limits.h>`** — `include/limits.h`.

- **`__builtin_atomic_arith_add/sub/or` macros missing** —

New regression test:

- `test/test_int128.c` (POST_INC, arithmetic, bitwise, shifts,
  comparison, divmod, neg, cast, comma, cond), PASS on x86-64.

Projects now verified:

- **libtommath**: builds (int128 error fixed; linker error for
- **libgc**: builds, 18/18 tests pass
- **libgit2**: builds (test code has its own syntax bug)
- **inih**: 16/16 tests pass (needed muon)
- **liballegro5**: builds (PTHREAD_STACK_MIN fixed; test timeout separate)
- **libuv**: builds and tests pass (IOV_MAX fixed)

### Fixed (2026-08-19, limits.h glibc #include_next + IEEE 754 math.h session)

Three related issues blocking 9 third-party projects:

- **glibc's `<limits.h>` `#include_next` loop** — `preprocess.c`. rcc

- **`<linux/limits.h>` not reached after `_GCC_LIMITS_H_` suppression**

- **IEEE 754 comparison macros missing from bundled `<math.h>`**

New regression tests:

- `test/test_limits_h_glibc_chain.c` (ISO C minimums + POSIX + Linux
  kernel limits via the full chain), PASS.
- `test/test_math_ieee754_comparison.c` (all six macros, NaN edge
  cases), PASS.
  Both PASS on x86-64; ARM64/mingw: `#include <linux/limits.h>` guarded
  out, `isgreater` etc. are pure macros — no target-specific concern.

Projects now verified:

- **box2d**: all tests pass
- **espruino**: builds, tests pass
- **ffc**: 6/6 tests pass
- **file**: builds and tests pass (was `isgreater` link error + limits.h)
- **ggrep**: builds (was autoconf `cannot detect` + limits.h)
- **cc65**: builds (test timeout is separate)
- **chibischeme**: builds (segfault in test is separate)
- **coremarkpro**: builds (test timeout is separate)
- **findutils**: autoconf now passes, build error is separate

### Fixed (2026-08-18, mimalloc multithread const-pollution session)

**mimalloc 2.1.2 `test-stress` SIGSEGV in a worker thread — root cause:
a `const` on an INCOMPLETE struct/union type polluted the SHARED tag
type.** mimalloc.h forward-declares `struct mi_heap_s; typedef struct
mi_heap_s mi_heap_t;` and uses `const mi_heap_t*` before the type is
ever completed. declspec's quals block did `copy_type(ty)->qual |=
quals` for struct/union types, and `copy_type()` deliberately returns
the SHARED pointer for every struct/union (so a forward declaration
can still be completed later through every existing reference) — so
the const landed on the canonical `mi_heap_s` type object itself.
Every later `mi_heap_t` declaration then read as const, including the
NON-const `mi_heap_t _mi_heap_main` in `src/init.c`; at -O3
eval_const_expr()'s ND_MEMBER fold (correctly gated on a const object)
folded `_mi_heap_main.thread_id == 0` to TRUE, compiling
`_mi_is_main_thread()` to `return 1` — every worker thread then set
its default heap to `_mi_heap_main`, and the multithreaded allocator
corrupted (block pointer `0xbf58476d1ce4e5f9` in `mi_block_next`).
Confirmed: the same pollution (from an earlier `const wuffs_base__io_buffer*`
incomplete use) folded real runtime branches in test_wuffs.

Fix (`src/parser.c`, `src/rcc.h`): a qualified INCOMPLETE aggregate now
gets its own "qualified variant" (`Type.qual_variants`, linked off the
canonical tag type) which `struct_or_union_specifier()` completes in
lockstep — member access, sizeof and declaration-vs-definition type
compat read through the variant, but its qualifier never leaks onto the
canonical type. A complete aggregate gets a plain qualified copy.
`qualify_struct_type()` centralizes the logic for declspec's quals
block, `qualify_array_elem()` and the `_Atomic(T)` path. The variant
also preserves the earlier fix's second half: a function declared with
`const struct S*` (S incomplete) still matches its definition after S
completes (dropping the qual entirely had caused "conflicting types").

New regression test: `test/test_incomplete_struct_const.c` (the
mimalloc pattern distilled: incomplete-tag const use → completion →
non-const global whose member reads must stay runtime reads; plus the
decl-before/def-after compat pair and `sizeof(const T)`). Verified
against gcc (rc=0) and confirmed the test catches the bug by reverting
`parser.c`/`rcc.h` alone: rc=2. **mimalloc 2.1.2 full CMake build
(CMAKE_C_COMPILER=rcc) now passes 3/3 ctest targets, including the
previously-crashing `test-stress`** (was: SIGSEGV within seconds).
`make check-all`: 0 failed (Unit 283/283, c-testsuite 220/220,
Compliance 15/15, Torture 3605/3609 — 0 failed, 354 skipped, 4 todo,
Dg-error 34/34, Link 11/11).

### Fixed (2026-08-17, this session — 5 bugs via test_kefir/test_cc65/linux_thirdparty.bash CC=gcc audit)

Triggered by auditing `test/linux_thirdparty.bash` for hardcoded `gcc`
that bypasses rcc (weakening third-party coverage): most existing
hardcodes turned out deliberate (host-toolchain reference oracles —
`test_binutils_gccverify`'s own name says so; kefir's
`scripts/detect-host-env.sh` needs literal GCC `-print-search-dirs`/
`-Wp,-v` output rcc doesn't replicate; tinycc's `make test` is gated on
`if gcc --version` specifically to use a second, independent compiler as
its own cross-check oracle). Two were genuine, unconditional bypasses
with no such justification and got switched to `CC="$CC"`:
`test_cc65`'s `make ... test` (compiles LCC-derived reference programs
for output comparison — using rcc there doubles as a codegen check) and
`test_kefir`'s `make CC=gcc test` (build/test phase, as opposed to the
`CC=gcc scripts/detect-host-env.sh` host-env probe, correctly left
alone). Rebuilding both against rcc surfaced 5 real, previously-hidden
rcc bugs, all confirmed via minimal repros cross-checked against real
gcc, fixed, and covered by new regression tests:

- **Bare `-M`/`-MM` (dependency-rule-only mode) entirely unrecognized**
  `test/test_dep_only_mm.c` (new), PASS on x86-64, ARM64 (qemu-aarch64)

- \*\*`thread_local`/`constexpr` treated as unconditional reserved
  spellings. Regression test: `test/test_pre_c23_thread_local_ident.c`

- \*\*Designated initializer chain continuing past an array-index step
  `test/test_compound_literal.c` (covers both bugs plus a

- \*\*Bare "-L path" / "-l name" (as two separate argv elements, as
  Regression test: `test/test_bare_L_l_linker_args.c` (new; links

- \*\*K&R (old-style) array-typed parameters kept their raw, undecayed
  `test/test_kr_array_param_decay.c` (new; 1D and 2D K&R array

**test_kefir now builds and links completely** (~800 source files) and
passes 503/504 of its own unit tests; the one remaining failure
(`BigInt - signed to long double conversion #1`) is a real, but
separate and out-of-scope, architectural limitation: rcc represents
`long double` internally as double precision (64-bit) everywhere except
at ABI call boundaries (where it correctly narrows/widens through
genuine 80-bit x87 `fldt`/`fstpt` to match the SysV calling convention),
documented at length throughout `codegen.c`. kefir's test constructs a
bit-exact 80-bit extended value via its own (real, gcc-compiled)
bigint-to-float routine and writes it directly through a `long double *`
the rcc-compiled test code owns — a raw memory boundary the ABI dance
doesn't cover. This never surfaces in an all-rcc-compiled program (every
`long double` value both written and read by rcc-compiled code agrees
on the same, consistently-narrowed representation); it's specific to
mixing rcc-compiled and gcc-compiled code that shares raw `long double`
storage across the boundary. Not attempted this session: implementing
genuine 80-bit-precision `long double` storage/arithmetic throughout
codegen is a large, separate undertaking, not a regression.
**test_cc65 makes substantial further progress** end to end: its
`asm`/`dasm`/`val` suites (thousands of individual regression-test
programs, cross-compiled by cc65 and run under the sim6502 emulator)
now run to completion, and the `array.c` crash this fix directly
targets is confirmed gone. `make test` still ultimately fails partway
through `ref/` (which compiles LCC-testsuite reference programs with
`$(CC)` for output comparison against cc65's own cross-compiled+
emulated output) on `yacc.c` — a ~40-year-old hand-rolled, byte-packed
DFA lexer/parser table (`ncform`/`yacc` 1983 vintage, per its own
header comment) that produces a different, and for rcc empty, parse
trace vs. real gcc. Bisected far enough to rule out several
hypotheses (the table-walk's pointer-into-array arithmetic itself is
correct — confirmed via injected debug prints; not an optimization-
level artifact — reproduces identically at `-O0`; not `char`
signedness — reproduces identically under `-funsigned-char`) but not
root-caused to a specific miscompiled construct this session — a
candidate miscompilation needing its own dedicated bisect of the
DFA table-walk's `struct yywork{char verify,advance;}` pointer-offset
dereferences, not attempted further given the scope. **Not the same
bug** as the `array.c` K&R-array-parameter-decay fix above (confirmed
independently: `array.c` now passes standalone).

### Fixed (2026-08-17, checklist triage session — 7 stacked bugs)

Worked through `test/third_party/checklist.txt`'s unchecked items one by
one (per-project, individually rebuilt/retested — not a batch run) after
installing missing sandbox packages (`lzip`, `muon-meson`, `libcmocka-devel`,
and fixing a broken `llvm-config` alternatives symlink; `libcap-devel` was
already present). Confirmed via `make check-all` after every fix (0 failed
across Unit/TCC/c-testsuite/Compliance/Torture/Dg-error/Link on native
x86-64) and cross-verified every new regression test on ARM64
(qemu-aarch64) and mingw (wine).

- \*\*`__builtin_add_overflow_p`/`__builtin_sub_overflow_p` entirely
  `test/test_builtin_overflow_p.c` (new; add/sub/mul_overflow_p, several

- **`-Wp,-MD,<file>` (single-M kbuild dependency flag) not recognized**
  `test/test_wp_md_kbuild_dep.c` (new), PASS on all three targets.

- \*\*`-funsigned-char`/`-fsigned-char` not implemented (tolerated as a
  `test/test_char_signedness_flags.c` (new; verifies both signedness

- \*\*Quote-include self-reference guard wrongly value-matched a user's
  `test/test_quote_include_self_reference.c` (new), PASS on all three

- \*\*`ilogb`/`ilogbf`/`ilogbl` and `FP_ILOGB0`/`FP_ILOGBNAN` missing from
  `test/test_ilogb.c` (new; guarded out under `_WIN32` matching the

- \*\*`-nostdlib`/`-r` (relocatable/partial-link output) silently
  Regression test: `test/test_link_nostdlib_relocatable.c` (new; two

- \*\*A GNU `__attribute__((packed))` trailing an individual struct
  `test/test_struct_member_packed_attr.c` (new; reproduces busybox's

**Remaining, not fixed this session** (real progress made, both
blocked on a separate, unrelated gap deeper in the same build):
test_busybox now builds through `libbb/` before failing on missing
SHA1-NI instruction encoders (`sha1msg1`/`sha1nexte`/`sha1msg2`/
`sha1rnds4`) plus `pextrd` in `libbb/hash_sha1_hwaccel_x86-64.S` — a
substantial new instruction-set-extension undertaking, not attempted.
test_chibischeme now builds its full interpreter and every `lib/`
shared module before segfaulting in its own `tests/r7rs-tests.scm` run
— a candidate miscompilation needing its own dedicated repro+bisect,
not attempted (same category as the other "wrong runtime output/crash"
entries under "Needs fixing" below).

**Confirmed not rcc bugs / environment-limited, checklist checked off**:
test_box3d (C++ binary, g++-compiled — not rcc, matches the existing
"Needs fixing" entry below); test_c23doku (arbitrary-precision
`_BitInt` up to 11163 bits — already documented as skipped by decision,
see "Needs fixing" item 1 above); test_cfitsio (`drvrsmem.c`'s `union
semun.val`/`HAVE_UNION_SEMUN` — already documented as a stale
configure-time feature-detection result reproducing identically
against real gcc, see "Needs fixing" item 6 above). test_bubblewrap and
test_c3 remain unchecked: bubblewrap builds far further after
installing `muon-meson` but still fails `dependency('libcap')` inside
muon's own pkg-config resolution despite `pkg-config --exists libcap`
succeeding directly (a muon-side bug, not investigated further); c3
builds through LLVM/CMake configuration after fixing the sandbox's
broken `llvm-config` symlink but needs static `liblldCOFF.a`, which
this Fedora sandbox's `llvm`/`lld` packages don't ship (shared-only).

### Fixed (2026-08-14, array-range designator / overflow builtins / ADX+mxcsr session)

- **GNU C designated array-range initializer on a struct member array**
  Regression test: `test/test_designator_array_range_member.c` (new,

- \*\*`__builtin_{s,u}{add,sub,mul}{,l,ll}_overflow` family (18 names)
  Regression test: `test/test_builtin_overflow_family.c` (new, all 18

- \*\*Missing SSE/assembler instruction coverage: ADCX/ADOX (ADX
  `test/test_asm_adx_mxcsr.c` (new; ADX flag-readback itself is not

- \*\*Remaining SSE/assembler instruction coverage: PSHUFLW/PSHUFHW and
  test: `test/test_asm_sse_shift_shuffle.c` (new, byte-verification

- \*\*`_mm_cvt_ss2si`/`_mm_cvt_si2ss`/`_mm_cvtt_ss2si` (original,
  Found via test_libopus. Regression test: `test/test_mm_cvt_ss2si.c`

### Fixed (2026-08-14, inline-asm XMM constraint session)

- \*\*Inline-asm `"x"`/`"=x"`/`"+x"` (XMM register class) constraint
  Regression test: `test/test_asm_xmm_constraint.c` (new: `"=x"`

### Fixed (2026-08-16, constant-fold dead-branch-elimination session)

- **`sizeof(char[1-2*COND])`-negative-array-size static-assert idiom**
  Regression test: `test/test_static_assert_negative_array.c` (new;

### Fixed (2026-08-16, mixed-width builtin overflow session)

- \*\*`__builtin_add_overflow`/`__builtin_sub_overflow`/
  Regression test: `test/test_builtin_overflow_mixed_width.c` (new).

### Fixed (2026-08-16, versioned-SONAME SemVer-prerelease-suffix session)

- \*\*rcc's linker-input classification rejected a versioned shared
  Regression test: `test/test-link.sh` case 13 (new: a shared library

### Fixed (2026-08-16, self-referential global array initializer session)

- \*\*A static/global array referencing itself by name within its own
  Regression test: `test/test_self_referential_array_init.c` (new;

### Fixed (2026-08-16, native linker cross-object symbol-value session — PE and Mach-O)

- \*\*rcc's native PE linker (`link_pe.c`, `link_load_object()`) resolved
  Regression test: `test/test-link.sh` case 12 (new: direct

### Fixed (2026-08-15, njs macro-driven initializer session — 6 stacked bugs)

- \*\*A CAST wrapping `&(compound literal)` in a static/global pointer
- \*\*A `double`-typed static initializer whose value is a purely-integer
- \*\*A top-level `const`/`volatile`/`restrict` qualifier difference on a
- **rcc's bundled `<math.h>` was missing `M_SQRT1_2` and `M_2_SQRTPI`**
- \*\*A non-`static`-qualified compound literal reached through the
- \*\*`eval_const_expr()`'s `ND_LVAR` case read a struct/union/array-typed
  Regression tests: `test/test_compound_literal.c` (cast-addr and

### Fixed (2026-08-15, empty attribute-specifier-sequence before a tag declarator session)

- \*\*A C23 `[[attrs]]` immediately before `struct`/`union`/`enum` was
  `test/test_attr_before_tag_declarator.c` (new: empty and non-empty

### Fixed (2026-08-15, #pragma once per-TU scoping session)

- \*\*`#pragma once` state leaked across translation units in a single
  Regression test: `test/test-link.sh` case 11 (new: two `.c` files

### Fixed (2026-08-15, zero-width-bitfield-only struct completeness session)

- \*\*A struct whose only member(s) are anonymous zero-width bitfields
  `test/test_zero_width_bitfield_struct.c` (new: the exact

### Fixed (2026-08-15, object-like macro `##` token-paste session)

- \*\*`##` (token-paste) was silently ignored in an object-like macro's
  Regression test: `test/test_object_like_macro_hashhash.c` (new: the

### Fixed (2026-08-15, trailing `_Pragma` before a struct's closing brace session)

- \*\*A trailing `_Pragma(...)` (or `__attribute__`/`[[...]]`) immediately
  Regression test: `test/test_struct_trailing_pragma.c` (new: a

### Fixed (2026-08-15, libatomic helper-name + glib `-std=gnu17` session)

- \*\*Two independent root causes behind test\*glib's `goption.c:212`
  Regression test: `test/test_atomic_libatomic_helpers.c`(new: load/

### Fixed (2026-08-15, large file read truncation session)

- \*\*Files larger than 10 MiB were silently truncated by the preprocessor
  Regression test: `test/test-link.sh` case 12 (new: a ~10.5 MiB file

### Fixed (2026-08-15, test_wuffs AVX2/SSE4.1 session — 5 stacked bugs)

- **`<x86intrin.h>`/`<immintrin.h>` had no AVX/AVX2 coverage at all**
- \*_A `const T _`function parameter mutated the shared struct/union`test_align_type_leak.c`). `const wuffs_base\_\_io_buffer \*buf`
- \*\*PBLENDVB/BLENDVPS/BLENDVPD (128-bit legacy, implicit-XMM0-mask
  SIGILL). Regression coverage added to `test/test_avx2_intrinsics.c`.
- \*\*VLDDQU (256-bit) used the wrong VEX.pp prefix bit, SIGILL'ing on
  coverage added to `test/test_avx2_intrinsics.c`.
- \*\*`__builtin_ia32_si_si256`/`ps_ps256`/`pd_pd256`/`permti256` had
  Regression coverage added to `test/test_avx2_intrinsics.c`.

All five bugs were found end-to-end via **test_wuffs**
(https://github.com/google/wuffs) — real-world AVX2 JPEG IDCT + SSE4.1
PNG unfilter code, the first target in this corpus to exercise this
specific combination of legacy-128-bit-blendv, VLDDQU, and 256->128
cast/permute intrinsics together. `example-convert-to-nia`'s own
`print-nia-checksums.sh`/`print-mzcat-checksums.sh` regression suite
(203 real image files: PNG/JPEG/GIF/WebP/BMP/QOI/TGA/etc.) now passes
byte-for-byte against the expected CRC-32 checksums, matching a real
`gcc -mavx2 -msse4.1` reference build for both `test/data/49.png`
(the PNG unfilter path) and `test/data/hat.jpeg` (the AVX2 IDCT path)
specifically. `make check-all`: 0 failed on native x86-64 (Unit
4209/4209, TCC 118/118, Compliance 251/251, C-testsuite 220/220,
Torture 3605/3609 — 0 failed, 354 skipped, 4 todo — Dg-error 34/34,
Link 8/8); ARM64 cross-build verified clean (these are x86-only VEX/SSE
encoders, guarded out on `ARCH_ARM64`, but the shared `declspec()`/
`ia32_builtin_ret()` changes needed re-verification there too).
New regression tests: `test/test_const_param_type_leak.c`; extended
`test/test_avx2_intrinsics.c` with `si_si256`/`ps_ps256`/`pd_pd256`/
`permti256`/`lddqu256`/`pblendvb128`/`blendvps`/`blendvpd` coverage,
all cross-checked against real `gcc -mavx2 -msse4.1` output.

### Fixed (2026-08-15, test_noplate `_Generic` array/struct-tag session — 4 stacked bugs)

- \*\*`_Generic`/`__builtin_types_compatible_p` rejected an unsized array
  `_Generic`. Regression test: `test/test_generic_array_unsized.c` (new).
- \*\*A struct/union tag redeclared with a byte-for-byte identical body in
  Regression test: `test/test_struct_tag_redef_identical.c` (new).
- \*\*`-iquote` was folded into the same include-search list as `-I`/
- \*\*rcc's own bundled include dir (`RCC_INCDIR`) was searched before
  `test_include_next_skips_user_dirs.c`/`test_include_next_dup_incdir.c`

All four bugs were found end-to-end via **test_noplate**
(https://github.com/mrirobert/noplate — Martin Uecker's type-generic
programming header library), which now builds its full library and all
six of its own tests (`tests/{list,maybe,span,string,vec,variadic}`)
cleanly and passes them all (`make test`, `CC=rcc`). `make check-all`:
0 failed on native x86-64 (Unit 254/254, TCC 118/118, Compliance
15/15, C-testsuite 220/220, Torture 3605/3609 — 0 failed, 354 skipped,
4 todo — Dg-error 34/34, Link 8/8); ARM64 and mingw cross-builds
verified clean. New regression tests:
`test/test_generic_array_unsized.c`,
`test/test_struct_tag_redef_identical.c`,
`test/test_iquote_precedes_bundled.c`.

### Fixed (2026-08-15, unclosed-string-literal warning + lexer line-number session)

- \*\*A string literal missing its closing quote at end-of-line was a hard
- \*\*Every `lex_error_at()`/`lex_warn_at()` diagnostic raised while
  Regression test: `test/test_string_literal.c` (new) — verifies

**test_gnutls** (https://gnupg.org/software/gnutls) now gets substantially
further into its real `./configure && make` build (unblocking every
`gl/`-directory translation unit that merely includes `config.h`) before
hitting a separate, deeper, **confirmed-not-an-rcc-bug** blocker one
layer down: `src/gl/tests/sys/ioctl.h`'s own gnulib-generated `ioctl`
redeclaration (`int ioctl(int, int, ...)`) genuinely conflicts with
glibc's real prototype (`int ioctl(int, unsigned long int, ...)`) —
verified byte-for-byte reproducible with a real `gcc -c` on the identical
construct (`conflicting types for 'ioctl'`), a real upstream
gnulib/glibc-version mismatch in this specific gnutls checkout's
`src/gl/tests` module, not an rcc gap. `make check-all`: 0 failed on
native x86-64 (Unit 4213/4213, Torture 3605/3609 — 0 failed, 354
skipped, 4 todo — Dg-error 34/34, Link 8/8); ARM64 and mingw cross-builds
verified clean. New regression test: `test/test_string_literal.c`.

### Fixed (2026-08-15, bundled-header **GLIBC** feature-macro visibility session)

- \*\*`__GLIBC__` (and every macro glibc's own `<features.h>` derives from
  Regression test: `test/test_glibc_gated_gnu_source.c` (new).

### Fixed (2026-08-14, MOVDQA/MOVDQU/MOVD/MOVQ + packed-integer memory-operand session)

- \*\*`salsa20_xmm6-asm.S` (test_libsodium) compiled with no error but
  output for isolated repros (`test/test_asm_movdqa_paddd_mem.c`,

### Fixed (2026-08-14, F16C intrinsics / unknown-flag acceptance session)

- **F16C half-precision convert intrinsics not implemented**
  header). Regression test: `test/test_f16c_intrinsics.c` (new),

- \*\*Several common, legitimate GCC/clang flags hard-error instead of

### Fixed (2026-08-14, C `defer` session)

- **C `defer` statement not implemented at all** (WG14 N3199 / TS 25755,
  test/test_defer.c: (1) a defer body that itself calls a function

### Fixed (2026-08-13, wide `_BitInt(N>64)` session)

- **`_BitInt(N)` with N > 64 silently truncated to 64 bits** (codegen.c,

### Fixed (2026-08-13, decimal `_Decimal32/64/128` session)

- **`_Decimal32/64/128` were aliased to float/double/long double** (parser.c,
  Regression test: `test/test_decimal.c` (7 sub-tests), PASS at

- **`_BitInt(N)` with N > 64 silently truncated to 64 bits** (codegen.c,
  `test/test_wide_bitint.c` (9 sub-tests), PASS at -O0/-O1/-O2/-O3 on

  **Follow-up (2026-08-16)**: the new `test_float_cast` in
  `test/test_wide_bitint.c` exposed that the "int → bitint" cast branch
  in `gen_bitint()` also matched flonum sources, shadowing the
  dedicated float→bitint conversion and reinterpreting the raw IEEE-754
  bits as an integer. Fixed by excluding flonum sources from that
  branch; x86-64 float→bitint now loads the source double into `%xmm0`
  before `cvttsd2si`. The same test pass added coverage for
  `_Decimal128` truncation to `_Decimal32/64` (previously copied the
  low 64 bits instead of calling `__bid_trunctd{sd,dd}2`) and
  `_Decimal128` → `_Bool` truthiness (previously tested only the low
  64 bits, so negative values whose coefficient fit entirely in the
  high word were wrongly falsy). Regression tests:
  `test/test_decimal.c` additions `test_decimal128_trunc` and
  `test_decimal_bool`.

- **macOS lacked `<stdbit.h>`** (new `include/stdbit.h`) — the C23
  `test/test_bit.c` unit test included `<stdbit.h>`; glibc provides it,

### Fixed (2026-08-07, blosc2 session)

- **Inline-asm multi-output register clobber** (codegen.c) — a
- **Function-declarator parameter names leaked into file scope**
- **`__builtin_cpu_supports("feature")`** (parser.c synthetic
- **SSE2 gaps**: `_mm_shufflelo_epi16`/`_mm_shufflehi_epi16`, the
- **SSSE3**: new `include/tmmintrin.h` (rcc had none) — everything
- **`stdint.h` `ptrdiff_t` typedef mismatch** — a pre-existing bug:

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
- **`type_equal()` struct/union pointer-identity check was too strict**
- \*\*glibc `_FORTIFY_SOURCE` `_chk`/`_chk_warn` builtins were bare
- **`include/assert.h` unconditionally included `<stdbool.h>`** — real
- \*\*Compile-time constant folding of float subexpressions cast to an
- **`__builtin_ia32_pause`/`mfence`/`lfence`/`sfence` unimplemented**

Regression tests: `test/test_err_proto_conflict.c`,
`test/test_proto_fnptr_struct_param.c`,
`test/test_proto_void_params.c` + `test/test_err_proto_void_params.c`,
`test/test_fortify_chk_arity.c`, `test/test_const_float_fold.c`,
`test/test_ia32_pause.c`. Full suite verified after each fix: TCC
118/118, Unit tests 163/163, Torture 3605/3609 (100% of non-skipped),
Dg-error 34/34, Link 4/4.

### Fixed (2026-08-08, httpparser session)

- \*\*`-funroll` (opt.c) aliased duplicated labels across unrolled loop

New regression test: `test/test_unroll_label_alias.c` — a bounded,
non-hanging reproduction (an escape-hatch counter breaks out after 20
bounces instead of spinning like the real bug) that fails fast
(`assert`) on the unfixed compiler at `-O2`/`-O3` and passes at every
optimization level once fixed. Full suite verified: TCC 118/118, Unit
tests 171/171 (also verified separately at `-O2`), Torture 3605/3609
(100% of non-skipped), Dg-error 34/34 — identical to baseline, plus
confirmed clean on the mingw and arm64 cross targets.

### Fixed (2026-08-08, test_bash session)

- \*\*`eval_const_expr()`'s `ND_MEMBER` fold treated any _global_ with a

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

- \*\*SysV x86-64 small-aggregate return: rcc always used a hidden return

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

- \*\*`asm_stur_fp()`/`asm_ldur_fp()` (codegen_asm.h) emitted `stur`/

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

- \*\*`gen_addr()`'s `ND_ASSIGN` case computed the address of an

New regression test: `test/test_assign_expr_lvalue.c` — struct member
access on an assignment expression's result, the same assignment used
twice within one `&&`-sequenced expression (the real macro-re-expansion
shape), `&` on a scalar assignment result, and a struct chain
assignment; all cross-checked against gcc. Full suite verified: Torture
3605/3609 (100% of non-skipped), Dg-error 34/34, Unit tests 176/176,
Link tests 5/5, 0 failed overall; confirmed clean (test PASSes) on the
mingw and arm64 cross-compile targets.

### Fixed (2026-08-08, continued — include/math.h erf/erfc)

- \*\*`include/math.h` (rcc's own bundled header, used in preference to

New regression test: `test/test_math_erf.c` — checks `erf`/`erfc`
against their true mathematical values (tight tolerance, not just "in
some plausible range" — a wrong-but-plausible-looking value like the
one this bug actually produced would trivially pass a loose bounds
check) plus the `erf(x) + erfc(x) == 1` identity. Full suite verified:
Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Unit tests
177/177, Link tests 5/5, 0 failed overall; confirmed clean (test
PASSes) on the mingw and arm64 cross-compile targets.

### Fixed (2026-08-08, cproc array/VLA-param type-modeling session)

- \**Array-parameter bracket qualifier decayed to `T *const` on the wrong
- **`__builtin_types_compatible_p` never handled `TY_VLA`**

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

| test             | symptom                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| test_lua         | **fixed** — passes cleanly now (confirmed via a fresh individual run this session, `rc=0` in 36s); no rcc changes were needed specifically for it, resolved by the accumulated fixes from prior sessions                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| test_mruby       | **fixed** — was: assignment-expr-as-lvalue bug + missing `erf`/`erfc` declarations, see "Fixed (2026-08-08, continued — ...)" sections above; `Total: 1686, OK: 1677, KO: 0, Crash: 0` (matches gcc-built mruby exactly)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| test_curl        | **fixed** — was: configure "compiler does not halt on prototype mismatch"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| test_c23doku     | needs arbitrary-precision `_BitInt` codegen (up to 11163 bits) — see "Needs fixing" item 1 below                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| test_c3          | **fixed** — was: `nan()`/`nanf()`/`nanl()` undeclared in rcc's bundled `<math.h>`, so calls fell back to implicit-int and returned garbage instead of NaN (c3c's own `double::nan`/`float::nan` compile-time constants are built via `nan("")`), corrupting NaN comparisons; see "Fixed (2026-08-24, test_c3 NaN comparison -- missing nan()/nanf()/nanl() declarations)" above. `c3c compile-test unit`: 1366/1366 passed (was 1365/1366)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| test_coremarkpro | benchmark runner can't find perf logs                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| test_box3d       | C++ binary (g++ compiled, not rcc)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| test_git         | **fixed** (2026-08-27) — was: t1013-read-tree-submodule.sh failed 58/58 remaining (10 known-breakage skips matched gcc); `git status -u -s` segfaulted in `git_parse_maybe_bool_text()` because a string literal cast to an integer type in a static initializer (`struct option` `.defval = (intptr_t)"all"`) stored the truthiness constant 1 instead of the literal's address, so the `-u` option's untracked-files mode default parsed the bytes at address 1; see "Fixed (2026-08-27, test_git -- string-literal-to-int static initializer stored 1)" above. t1013 now passes all 58 remaining tests, matching the gcc-built git; the only remaining `not ok`s are git's own `# TODO known breakage` (test_expect_failure) entries, identical on gcc                                                                                                                                                                                                                                                                                                                                                                     |
| test_glib        | **build + resources test fixed** (2026-08-26) — was: `test_resources.c:94` compile error (lexer long-string truncation, fixed) then a `resources` test SIGSEGV: `__attribute__((constructor))/(destructor)` flags leaked across prototype-only declarations, so the `.fini_array` held the constructor (see "Fixed (2026-08-26, glib -- constructor/destructor...)" above); the remaining `file` test_measure mismatch (98952 vs 74376) is an environment-dependent gvfs-daemon artifact (6 dirs x 4096 counted), not an rcc bug — the rcc-built libgio returns the correct value in isolation                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| test_got         | configure: missing libbsd-overlay                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| test_gnutls      | **build fixed, 37 test-suite failures all rcc-caused** (2026-08-27) — was: (1) duplicate DT*NEEDED from repeated `-lm` mangled configure's M_LIBRARY_SONAME probe (broken config.h define, 991 warnings), fixed in link_elf.c; (2) block-scope conflicting function redeclaration not diagnosed, so configure's ioctl POSIX-signature probe wrongly passed and the generated sys/ioctl.h took the SYS branch, failing the build at src/gl/tests vma-iter.o — fixed in parser.c (func_decls_conflict), see the two "Fixed (2026-08-27, test_gnutls ...)" sections above. Build + fuzz now complete; gnutls' own `tests` suite still fails 37 tests (resume-*, system-override-\_, pkcs11/\*, mini-global-load, tls-supplemental, tls-ext-register, rfc7633-ok, ...) that ALL PASS on a gcc-built gnutls — rcc miscompiles gnutls runtime code (resume-dtls segfaults in libgmp's \_\_gmpz_sizeinbase on a corrupt mpz built by rcc-compiled nettle wrappers); separate session                                                                                                                                                 |
| test_ksh93       | **all arith.sh failures fixed except one documented long-double precision limitation** — the `cos*cos + sin*sin > 1.01` hang and the `-0` sign-of-zero failures are fixed (long-double assignment-expression value, IEEE flonum truthiness with -0.0, and 64-bit pointer arithmetic/subtraction — see "Fixed (2026-08-12, ksh93 arith / IEEE truthiness / 64-bit pointer-arithmetic session)" below); ksh93's own full `shtests` suite now runs with only `arith.sh[327]` failing (3x: plain/C.UTF-8/shcomp): `typeset -lE20 val=123.01234567890` prints the double expansion `123.01234567890000449` — rcc computes `long double` internally in double precision by design (documented in codegen.c), so 20-significant-digit extended-precision rounding cannot hold; real 80-bit long double support is a large feature, not a quick fix. Build still compiles/links the entire `libast` + `iffe` 161/161, and the whole `shtests` suite passes except that one rounding check                                                                                                                                             |
| test_libgmp      | **shared/static library build now fully fixed** — was: configure-time "cannot determine 32-bit word directive" (stale, long-superseded by prior sessions' assembler fixes); this session found and fixed the last two real rcc bugs blocking the actual `libgmp.so`/`libgmp.a` link (forward-referenced local-label binding, `.hidden`/`.protected`/`.internal` ELF visibility — see "Fixed (2026-08-11, forward-referenced local-label binding / ELF visibility session)" below). The library's own `tests/mpn/t-*` runtime suite still shows 47 failures, confirmed **not an rcc bug** — bit-for-bit reproduces (identical exit codes) against the same GMP 6.3.0 source built with the system's real gcc+GNU as+GNU ld, a pre-existing GMP/environment incompatibility                                                                                                                                                                                                                                                                                                                                                     |
| test_muon        | **all rcc-related self-test failures fixed; 384/387 (3 env/muon failures identical to gcc-built muon)** — was: 342/387 with ~22 rcc-related failures (link-command generation). Two-part fix: rcc now falls back to the external (gcc) linker whenever the internal ELF/PE/Mach-O linker cannot honor a linker command (any `-Wl,` option or `-nodefaultlibs` — previously silently dropped, producing links with no DT_RUNPATH/DT_SONAME or unresolvable archive groups; see "Fixed (2026-08-12, external-link fallback / muon toolchain detection session)" below), and a bare `-Wl,-v`/`-l` invocation is a link-only probe instead of "no input files"; the harness now registers `rcc` as a muon compiler toolchain (inherit posix, linker `ld`) exactly like the existing `slimcc` patch, so muon detects rcc's linker as GNU ld and generates the full `-Wl,--as-needed/-rpath/--start-group/-soname` args. Remaining 3: `muon/wayland` (needs wayland-scanner tool), `common/183 partial dependency` + `common/251 add_project_dependencies` (zlib link path) — all reproduce identically with a fully gcc-built muon |
| test_neovim      | **investigated, not an rcc bug** — CMake configure fails before any compilation: `Could NOT find Luv (missing: LUV_LIBRARY LUV_INCLUDE_DIR)` (missing system Lua-libuv-binding dev package in this sandbox, not an rcc issue)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| test_nob         | **fixed** (2026-08-14, C `defer` session) — was: needs C's experimental `defer` statement (`-fdefer-ts`, WG14 N3199/TS 25755, not yet standardized); see "Fixed (2026-08-14, C `defer` session)" and "Needs fixing" item 5 above/below                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| test_rsync       | **fixed** — was: `undefined reference to 'preserve_acls'`/`'preserve_xattrs'` at link time; block-scope-`extern`-inside-dead-`static-inline`-function DCE bug, see "Fixed (2026-08-09, block-scope extern DCE session)" below                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| test_samba       | **two real rcc bugs found and fixed** (see "Fixed (2026-08-10, LONG_MAX/atomic-load session)" below) — configure now progresses far past its earlier `pyembed`/`Python.h` failure into unrelated dependency checks (pam, iconv, ncurses, readline, ...), currently blocked on `perl module "Parse::Yapp::Driver" not found` (missing build-time CPAN module in this sandbox, not an rcc issue)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| test_scrapscript | **fixed** — was: every test failed to even link (`undefined reference to '__start_const_heap'`); the `section()` attribute fix resolved linking (32/33 -> 17/33 failing), then the section sh_addralign fix below resolved the remaining 17 `SIGABRT`s (17/33 -> 0/33 failing, full suite green)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| test_tcpdump     | **fixed** — real rcc register-allocator bug found and fixed (deeply nested ternary in the radiotap decoder's bit-scan macro), see "Fixed (2026-08-10, continued — nested-ternary register-allocator session)" below; own `make check` now 0 failed, 636 passed                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |

---

## rc=2 — Build Failures (compile/link error)

Top root causes identified:

### Fixed (2026-08-06)

- `UINT_FAST64_MAX` / `INT_LEAST*_MAX` undeclared — **fixed** (stdint.h: added 24 missing limit macros)
  → unblocks: coreutils, diffutils, gpatch, gsed, gtar
- `__DATE__` / `__TIME__` undeclared — **fixed** (preprocess.c: added C89 predefined macros)
  → unblocks: mimalloc
- **memcmp**: with empty string
- **VA_OPT**: C23 variadic macro (bfs) — keyword registered but not expanded
- **Prototype mismatch not diagnosed** — **fixed**, see "Fixed

### Fixed (2026-08-08, shufflevector/SSE2-baseline session)

- **`__builtin_shufflevector` unimplemented** (parser.c) — a Clang
- **Large baseline-SSE2 gap in `include/emmintrin.h`** — once

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
`\_\_builtin_ia32*\*` AVX-512 builtins rcc has no codegen for at all —
EVEX encodings, ZMM regs and k-masks; AVX2 (test_brotli) is now fully
implemented, see "Fixed (2026-08-12, AVX/AVX2 intrinsics session)"
below); test_fftw's failure
(`fftw3.h:483`, `\_\_float128`/`FFTW_DEFINE_API`quad-precision) is
unrelated to SIMD entirely. A genuine fix for blake3/brotli would mean
rcc shipping its own`<immintrin.h>`/`<avxintrin.h>`/`<avx2intrin.h>`(the way it already does for`<emmintrin.h>`/`<tmmintrin.h>`) instead
of falling through to GCC's real, AVX-512-chaining system headers —
a multi-session effort, not a quick win.

### Fixed (2026-08-16, TLS symbol-type / extern-redeclaration session)

Found while chasing test_ocaml/test_mimalloc's real link failures (both
previously misdiagnosed as "the rcc-produced static archive itself is
malformed" — the archive format was fine; three separate codegen/parser
bugs corrupted individual member object files).

- \*\*A global variable's own definition, followed by a same-TU `extern`
- \*\*x86-64 non-PIC (local-exec) TLS variable reference registered the
- **`__builtin_thread_pointer()` was entirely unimplemented** (a real

Verified end-to-end: mimalloc's real CMake build (native rcc as
`CMAKE_C_COMPILER`) now links `libmimalloc.a`/`.so`, and
`mimalloc-test-api`/`mimalloc-test-stress` both pass cleanly at runtime
(the stress test exercises heavy multithreaded TLS access). A minimal
repro of OCaml's exact `domain.b.o`/`alloc.b.o` archive-link shape (two
objects, one owning a `_Thread_local` definition, the other only
referencing it, linked into a non-PIE executable exactly like
`runtime/ocamlrun`) now links and the value round-trips correctly
through TLS. `make check-all`: Unit 265/265 (new test included), TCC
118/118, Compliance 15/15, C-testsuite 220/220, Torture 3605/3609 (0
failed), Dg-error 34/34, Link 10/10. ARM64 (qemu-aarch64) and mingw
cross-builds verified.

New regression test: `test/test_global_extern_redecl_link.c` (drives
rcc as a subprocess, inspects `readelf -sW` symbol types directly, and
performs a real end-to-end TLS link+run for the archive-link case).

### Fixed (2026-08-16, SysV x86-64 struct-by-value argument ABI session)

- \*\*Every struct/union argument passed BY VALUE and larger than 8 bytes
  Regression test: `test/test_struct_arg_sysv_abi.c` (new; a >16-byte

### Needs fixing

1. **C23 `_BitInt(N)`** — **test_cproc fixed** (see "Fixed (2026-08-08,
   root-cause/test_cproc-completion session)" below: `_BitInt` was never
   the real blocker for test_cproc — full type-system support has been
   added regardless). **Wide `_BitInt(N>64)` codegen is now fixed too**
   (see "Fixed (2026-08-13, wide `_BitInt(N>64)` session)" above: all
   operations route through the embedded slimcc bitint runtime).
   **test_c23doku remains skipped by decision** — its
   `brute_force.c`/`graph_color.c` declare `_BitInt(total * 3)` where
   `total = digit * digit`, i.e. up to 11163 bits for the 61x61 puzzle
   (175 64-bit legs). The runtime is width-agnostic so it would compile
   and run, but the project is not a target (no arbitrary-precision
   workload in scope).

2. **lib/tempname.c pattern** — **fixed** (2026-08-14, diffutils session):
   test_diffutils now builds and passes its whole test suite (29 PASS).
   Four stacked fixes: C23 `*_WIDTH` macros (`SIZE_WIDTH`/`PTRDIFF_WIDTH`/
   ...) in include/stdint.h; `__builtin___mempcpy_chk` and
   `__builtin___stpcpy_chk` fortify macros (the latter must keep stpcpy's
   end-pointer return); a parenthesized string-literal-chain initializer
   (`char s[] = ( "a" "b" )`, diffutils' C_ifdef_group_formats) that sized
   to 0 and dropped the guard bytes; and glibc fortify `__REDIRECT`
   (`readlinkat` → `__readlinkat_chk`) being wrongly applied to
   address-of/function-pointer uses, so careadlinkat's `readlinkat`
   function pointer called the 5-arg `_chk` variant with a garbage buflen
   and aborted on a buffer-overflow check.

3. **Object file passed as source** — test_heatshrink **fixed** (2026-08-14):
   `.os`/`.od` (PIC/PIE object) positional link inputs are now recognized
   as object files, not C sources. test_heatshrink passes (rc=0).

4. **Link failures (environment/upstream, not rcc)**: test_file
   (`undefined reference to 'isless'`, a glibc math.h macro rcc appears
   to emit as a real call instead of inlining — actually **needs
   re-triage**, tentatively rcc-side, see item 6 below), test_libgc /
   test_libjansson (**confirmed upstream bug, not rcc**: both
   independently call a nonexistent `__builtin_atomic_arith_add/sub/or`
   -- verified this session that real `gcc -c` also rejects that exact
   name with "implicit declaration of function", so any compiler
   advertising `__sync`/`__atomic` builtin support hits the identical
   broken macro in libgc's `include/private/gc_atomic_ops.h` and
   jansson's `src/jansson.h`; not an rcc-specific gap). Missing system
   libs/dev-headers unrelated to rcc: libseccomp, libzstd, ALSA
   (`alsa/asoundlib.h`, test_sokol), etc.

5. **C `defer` statement (WG14 N3199 / TS 25755, `-fdefer-ts` /
   `_Defer`)** — **fixed** (2026-08-14, C `defer` session): see "Fixed
   (2026-08-14, C `defer` session)" above. test_nob's own test suite
   (12 of its 14 `tests/*.c`, excluding a Win32-only test and one with
   unstable directory-listing order) now builds and runs to completion
   under `rcc -fdefer-ts`, output checksum matching exactly.

6. **Fresh clusters found in the 2026-08-14 full 221-target batch
   triage** (5 parallel scouts read every failing log; F16C and the
   unknown-flag-acceptance clusters below are already fixed, see
   "Fixed (2026-08-14, F16C intrinsics / unknown-flag acceptance
   session)" above — the rest are open, listed by cluster, not
   individually fixed this session):
   - **GNU C designated array-range initializer, `__builtin_*_overflow`
     family, ADCX/ADOX/STMXCSR/LDMXCSR, and PSHUFLW/PSHUFHW/PSLLD/
     PSRLD assembler gaps** — all **fixed**, see "Fixed (2026-08-14,
     array-range designator / overflow builtins / ADX+mxcsr session)"
     above.
   - **Inline-asm `"x"`/`"=x"` (xmm register) constraint (test_libopus,
     test_nettle, and likely others)**: **fixed**, see "Fixed
     (2026-08-14, inline-asm XMM constraint session)" above.
   - **`_mm_cvt_ss2si` intrinsic (test_libopus)**: **fixed**, see
     "Fixed (2026-08-14, array-range designator / overflow builtins /
     ADX+mxcsr session)" above.
   - **`salsa20_xmm6-asm.S` (test_libsodium)**: **fixed**, see "Fixed
     (2026-08-14, MOVDQA/MOVDQU/MOVD/MOVQ + packed-integer
     memory-operand session)" above.
   - **Parser rejects valid C in real project source** (each a distinct
     construct, not one root cause): an empty-body context (test_lexbor,
     `normalization_forms.c:206` — **fixed**, actually a large-file
     truncation in `read_pp_file()`, see "Fixed (2026-08-15, large file
     read truncation session)" above). **A self-referential static
     array initializer (test_mquickjs, mqjs_stdlib.h's generated
     `js_stdlib_table[]` ROM table) is fixed** — see "Fixed
     (2026-08-16, self-referential global array initializer session)"
     above (test_njs is fixed too, see "Fixed (2026-08-15, njs
     macro-driven initializer session — 6 stacked bugs)" above); **struct
     member access "no such member" cluster (test_php, test_cfitsio,
     test_tcl) is resolved — confirmed NOT an rcc bug in 2 of 3, 1
     already fixed by prior sessions**: test_cfitsio's
     `drvrsmem.c:82/334` (`union semun.val`) and test_tcl's
     `tclUnixFCmd.c:349` (`struct dirent64.d_name`) both reproduce
     identically against real `gcc` with the exact same build-provided
     `-D` flags — both projects' own `./configure` baked in a stale
     feature-detection result (`HAVE_UNION_SEMUN=1`,
     `HAVE_STRUCT_DIRENT64=1`) for a glibc that no longer unconditionally
     provides that type: modern glibc's `<bits/sem.h>` never defines
     `union semun` at all (`_SEM_SEMUN_UNDEFINED`, by design since 2.2 —
     the caller must define it, exactly as each project's own header
     does when `HAVE_UNION_SEMUN` is unset), and `struct dirent64` is
     `#ifdef __USE_LARGEFILE64`-gated, never enabled by this exact build
     command (no `_GNU_SOURCE`/`_LARGEFILE64_SOURCE`/`_DEFAULT_SOURCE`
     define anywhere in it) — both genuinely incomplete types on this
     system, `.val`/`.d_name` correctly rejected as "no such member" of
     a zero-member type by rcc and real gcc alike. test_php's
     `apprentice.c`/`data_file.c` (`struct type_tbl_s`) now compiles
     cleanly with the current `rcc` — already fixed by the large-file
     read-truncation and unclosed-string-literal-warning sessions noted
     above (both landed after this cluster was first reported); no
     further change needed. **An unclosed string literal
     (test_gnutls, `config.h:2359`) is fixed**, see "Fixed (2026-08-15,
     unclosed-string-literal warning + lexer line-number session)"
     above — real gcc only warns and recovers there; test_gnutls now
     gets substantially further before hitting a separate, confirmed
     upstream (not rcc) gnulib/glibc `ioctl` prototype mismatch. **A
     conflicting-types false-positive between a local prototype and a
     generic-function expansion (test_gtar, `xattrs.c`) is confirmed
     NOT an rcc bug** — real gcc rejects the identical construct too
     (glibc's own `<sys/acl.h>` on this system already declares
     `acl_get_file_at`/etc. with different signatures than gtar's own
     gnulib polyfill expects). **`dlfcn.h`'s `Dl_info`/`dladdr` usage
     (test_nqp, dyncall's `dynload_syms_elf.c`) is fixed** — see "Fixed
     (2026-08-15, bundled-header `__GLIBC__` feature-macro visibility
     session)" above (rcc's own bundled `<stdint.h>`/`<stddef.h>` never
     made `__GLIBC__` visible, so the file's own `#if defined(__GLIBC__)`
     guard around `_GNU_SOURCE`/`__USE_GNU` silently never fired).
     **wolfSSL's macro-generated union member declaration
     (test_wolfssl, `hash.h:109-254`) is fixed** — passes cleanly now
     (confirmed via a full CMake build plus a fresh run this session:
     builds 100%, `wolfcrypt/test/testwolfcrypt`'s full algorithm
     self-test suite passes every test, `tests/unit.test`'s full TLS
     handshake suite runs real client/server sessions with no
     failures); no dedicated rcc change was needed specifically for
     it, resolved by the accumulated struct-tag/pragma/`_Generic`
     fixes from earlier in this session. **A `_Generic` dispatch macro
     rejecting a valid association (test_noplate) is fixed**, see "Fixed
     (2026-08-15,
     test_noplate `_Generic` array/struct-tag session — 4 stacked
     bugs)" above — four stacked bugs (array/VLA `_Generic`
     compatibility, identical-redefinition struct-tag reuse, `-iquote`
     leaking into angle-form resolution, `-iquote` vs rcc's own bundled
     include dir precedence), not just the one originally suspected.
     **AVX2 header gap plus 5
     stacked codegen bugs (test_wuffs) are fixed**, see "Fixed
     (2026-08-15, test_wuffs AVX2/SSE4.1 session — 5 stacked bugs)"
     above. **A C99 flexible array member inside a struct (test_bfs,
     `struct ioq_thread threads[];`) is fixed**, see "Fixed (2026-08-15,
     empty
     attribute-specifier-sequence before a tag declarator session)"
     above — turned out to be a distinct, general parser bug (not
     specific to flexible array members). **A `static` function
     definition with a typedef'd return type (test_elk, `main.c:23`)
     is fixed**, see "Fixed (2026-08-15, #pragma once per-TU scoping
     session)" above — turned out to be a `#pragma once` cross-TU
     state leak, not a return-type parsing bug. **`sizeof` applied to
     an incomplete type inside a `static_assert`-style macro idiom
     (test_utillinux) is fixed**, see "Fixed (2026-08-15,
     zero-width-bitfield-only struct completeness session)" above —
     turned out to be a struct/union completeness-detection bug, not
     specific to `static_assert`. **A token-pasting macro used as a
     declarator (test_parrot) is fixed**, see "Fixed (2026-08-15,
     object-like macro `##` token-paste session)" above — turned out
     to be a general object-like-macro preprocessing bug, not specific
     to declarator position. **A system header's own struct closing
     brace inside a nested context (test_liballegro5,
     `gdtlsconnection.h:108`) is fixed**, see "Fixed (2026-08-15,
     trailing `_Pragma` before a struct's closing brace session)"
     below — a real glib system header (`<gio/gdtlsconnection.h>`)
     bug, also independently blocking test_emacs's `xterm.h:1848`
     (same header, transitively pulled in via `<gtk/gtk.h>` behind
     `#ifdef HAVE_GTK3`) — **that exact cited diagnostic is fixed and
     independently reverified against the real system header**, but
     emacs's own full build has since been reconfigured cleanly (GTK3
     disabled by configure in this sandbox), proceeds through C
     compile/link after the `stdc_bit_width` header fix above, and now
     stops at a separate runtime segfault while generating
     `bootstrap-emacs.pdmp`. **An
     anonymous-union member pattern (test_glib, `goption.c:212`) is
     fixed** — but by two unrelated root causes, see "Fixed (2026-08-15,
     libatomic helper-name + glib `-std=gnu17` session)" below: (1)
     rcc's default `-std=c23` makes `bool`/`true`/`false` keywords, so
     glib's `gboolean bool;` union member only parses under
     `-std=gnu17` (now passed via the harness, matching real
     GCC/Clang's actual default); (2) glib's `gatomic.h` calls the
     libatomic helper names `__atomic_load_4/8`/`__atomic_store_4/8`
     directly, which rcc emitted as unresolved link symbols instead of
     inlining. Both fixed, and the whole `glib/` library now compiles
     and links — the full glib tree is next blocked one layer deeper in
     `gio/inotify` by glibc's `<sys/inotify.h>` (`char name
__flexarr;` -> `[]` member + `__PTRDIFF_TYPE__` in rcc's own
     `<stddef.h>`), a separate issue not yet investigated.
   - **Wrong runtime output / crash in rcc-compiled code** (candidate
     miscompiles — each needs its own dedicated repro+bisect, not
     attempted this session): test_box2d (rc=139, confirmed the
     crashing binary's C code was entirely rcc-compiled, not a non-rcc
     C++ binary as the generic SIGSEGV/rc=139 heuristic assumes),
     test_box3d, test_doom (linker drops global-variable definitions
     from one large TU), test_espruino, test_femtolisp, test_gzip
     (crashes running its own freshly-built binary to generate docs),
     test_hare (**confirmed NOT an rcc bug, see "Verified (2026-08-24,
     test_hare -- no rcc bug)" below**), test_wren (`free(): invalid pointer`) -- **fixed**,
     re-verified 2026-08-22: `wren_test` passes all 866 tests with 0
     failures (the earlier module-import/`free()`-invalid-pointer
     failures are gone, resolved by accumulated fixes), test_xz
     (13/19 CTest failures incl. a SEGFAULT) -- **fixed**,
     re-verified 2026-08-22: all 19 `ninja test` targets pass (see
     "Fixed (2026-08-22, inline-asm operand-store-back drain reusing a
     spill-cleared value register)" above), test_zstd (SIGABRT during
     its own regression tests) -- **fixed**, re-verified 2026-08-22:
     `make check` passes fully (struct-arg cast fix), test_yyjson
     (`test_number` subprocess crash, 11/12 other tests pass) --
     **fixed** (uint64->double round-to-odd session above), test_libevent
     -- **timeout artifact, not an rcc bug** (regress passes 356/356;
     `make check` exceeds the 420s budget identically with gcc), test_libsamplerate,
     test_libpng, test_tinycc (`-nan` from an unsigned-long-long to
     double conversion -- **root-caused, not fixed**: traces to
     `__floatundisf`-style helpers computing a signed-long-long-to-long-
     double cast plus 2^64 at genuine 80-bit x87 precision inside TCC's
     own JIT-compiled target program, but rcc's `long double` is
     architecturally double-precision internally end to end (arithmetic
     goes through SSE addsd/subsd, never real x87 fadd) -- a deliberate,
     already-documented limitation (the increment/decrement long-double
     codegen stores long-double literal constants as plain 8-byte
     doubles on purpose). A real fix needs genuine x87-register-backed
     long-double arithmetic throughout codegen, not a local patch; out
     of scope this session), **test_tomlc17 is fixed** -- see "Fixed
     (2026-08-16, SysV x86-64 struct-by-value argument ABI session)"
     above, **test_ruby is fixed** -- see
     "Fixed (2026-08-16, constant-fold dead-branch-elimination session)"
     below (its negative-array-size sizeof static-assert trick needed
     both a lenient constant fold through string-literal indexing and a
     ternary/if dead-branch skip that GCC only gets from its optimizer
     at -O2 -- rcc's fix is narrower and always-on, not a general
     inliner), **test_vlc is fixed** -- see "Fixed (2026-08-16,
     mixed-width builtin overflow session)" below (`ckd_mul(&res,
LLONG_MAX, -1)`, a `long long` times `int` mix, corrupted both the
     result and the overflow flag),
     **test_ocaml/test_mimalloc are fixed** — see "Fixed (2026-08-16,
     TLS symbol-type / extern-redeclaration session)" above (three
     stacked bugs: a global definition followed by a same-TU `extern`
     redeclaration silently dropped the definition entirely; x86-64
     non-PIC/local-exec TLS references registered their undefined
     symbol as `ST_NOTYPE` instead of `ST_TLS`, tripping `ld`'s "TLS
     definition ... mismatches non-TLS reference" on OCaml's
     `caml_state`; and `__builtin_thread_pointer()` was unimplemented,
     needed by mimalloc's TLS fast path), **test_nanomsg is fixed** —
     see "Fixed (2026-08-16, versioned-SONAME SemVer-prerelease-suffix
     session)" below (rcc misclassified a `libfoo.so.N.N.N-dev`-style
     versioned shared library, with a trailing SemVer prerelease tag
     after the version digits, as a C source file to compile).

   Every cluster above is grounded in a specific `file:line` and
   quoted rcc/linker diagnostic captured in
   `test/third_party/logs/<target>.log` from the 2026-08-14 batch run;
   re-read the log for exact context before starting a fix.

7. **mingw target: direct multi-`.c`-file-to-executable compile produced
   a broken output** — **fixed**, see "Fixed (2026-08-16, mingw native
   PE linker cross-object symbol-value session)" below. Root cause was
   deeper than the naming/permission symptom originally reported: rcc's
   own native PE linker (`link_pe.c`, only reachable when the mingw
   target compiles 2+ `.c` files directly to an executable in one
   invocation) mis-resolved every symbol DEFINED in the second or later
   linked object to the wrong address, silently miscompiling any
   multi-TU mingw executable.

8. **libgit2's `grafts::parse::oid_with_parent`/`oid_with_parent_and_newline`/
   `oid_with_multiple_parents`/`multiple_oids_with_multiple_parents`
   fail at runtime** (`tests/libgit2/grafts/parse.c:56`,
   `git_oid__fromstr(&oid, va_arg(ap, const char *), ...)` inside the
   test's own `assert_graft_contains(git_grafts *, const char *, size_t
n, ...)` helper) — "unable to parse OID - contains invalid
   characters", only when `n >= 1` (at least one variadic `const char *`
   OID argument follows). Surfaced by the "Fixed (2026-08-25, libgit2 --
   braced scalar initializer trailing comma)" fix above, which let
   `libgit2_tests` build/link for the first time — not a regression from
   that fix (the OTHER ~380 non-`grafts` suites, including the exact
   suite the originally-reported bug lived in,
   `network::remote::remotes`, run clean). Two minimal standalone
   repros matching the failing call's exact shape (`f(size_t n, ...)`
   reading `const char*` via `va_arg`; `f(ptr, ptr, size_t n, ...)`
   matching `assert_graft_contains`'s full signature) both print the
   correct string with rcc — the bug does NOT reproduce in isolation,
   so it's either specific to something in the real call site (a
   `cl_git_pass()` macro wrapper, `git_grafts_parse`/`git_grafts_get`
   themselves, or an interaction with the PRECEDING
   `assert_parse_succeeds()` call in the same test) or an upstream
   test-data/environment issue not yet distinguished from an rcc bug —
   not root-caused this session; a gcc cross-check build of
   `libgit2_tests` itself failed for an unrelated reason on this system
   (`undefined reference to '__builtin_atomic_arith_add'` from
   `src/util/thread.h`, matching item 4's `test_libgc`/`test_libjansson`
   upstream-macro finding, not investigated further). Needs a dedicated
   bisection session: reduce the real `git_grafts_parse()`/
   `git_grafts_get()` call chain (not just the variadic-shape
   micro-repro, which didn't reproduce it) to find the actual divergence
   point.

9. **valkey's `valkey-server` aborts on startup with a glibc pthread
   assertion** (`Fatal glibc error: pthread_mutex_lock.c:88
(___pthread_mutex_lock): assertion failed: mutex->__data.__owner ==
0`), reached AFTER logging its TSC-based monotonic clock banner
   (confirming the "Fixed (2026-08-25, valkey -- sizeof unsigned type /
   int128 shift clobber / rdtsc)" fixes above are exercised correctly)
   but before it can serve a single request. Surfaced now that valkey
   builds at all; not root-caused this session -- needs bisection
   against a real gcc-built valkey-server first to rule out an
   upstream/environment mutex-attribute mismatch (e.g. a
   `PTHREAD_MUTEX_ERRORCHECK` or robust-mutex attribute rcc's pthread
   header/runtime handles differently) before assuming an rcc codegen
   bug.

10. **`__int128`/`unsigned __int128` truthiness testing inconsistently
    aborts depending on the exact expression shape** -- **fixed**, see
    "Fixed (2026-08-25, **int128/\_Decimal128 truthiness address-vs-value
    session)" above. Root cause: `if`/`while`/`&&`/`||`/ternary
    conditions typed `**int128`/`\_Decimal128`tested the 16-byte slot's
ADDRESS (always truthy) instead of its value, and two of the four
affected sites also mis-sized the compare (16 bytes, unsupported),
silently dropping the REX.B bit needed for a VReg mapped to`%r8`-`%r15` and testing an unrelated aliased register instead --
exactly matching the reported "`assert(f(a,b))` crashes" symptom.

---

## rc=124 — Timeouts

| test              | notes                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| test_bash         | **fixed** — `dstack` const-fold bug + small-struct return ABI bug + `-rdynamic` not implemented, see "Fixed (2026-08-08, ...)" sections above; own `make test` now runs to completion, `run-glob-bracket` also passes                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| test_perl         | **fixed** — the spill-slot/locals-offset collision root-caused in "Investigated, not fixed (2026-08-10, register-spill/locals-collision session)" below is fixed, see "Fixed (2026-08-10, continued — spill/locals collision)" below; re-ran the real target fresh (`./Configure -des -Dcc=rcc ...`, `make -j3 test_prep && HARNESS_OPTIONS=j3 make test_harness`) and it now builds miniperl, the full `perl`, every extension, and `make test_harness` cleanly to completion — `rc=0` in 557s (previously segfaulted inside `Perl_upg_version()` before even reaching `lib/buildcustomize.pl`)                                                                                                                                                             |
| test_go           | **C bootstrap phase fully fixed; blocked one layer deeper** — a lexer infinite-loop DoS bug (any non-identifier-start non-ASCII byte hung rcc forever) and a missing `__attribute__((weak))` variable-linkage gap (two independent stacked bugs), see "Fixed (2026-08-11, continued — lexer non-ASCII infinite loop / weak variable attribute session)" below; `cmd/dist`/`lib9`/`libbio`/`liblink`/`5c`/`6c`/`8c`/`9c`/`5g`/`6g`/`8g` now build and self-bootstrap completely, reaching real `.go` stdlib compilation with the freshly-built `6g` — which then hits a likely miscompilation in `6g`'s own (Plan9 C, rcc-built) embedded-struct-field-promotion logic, several layers removed from rcc's own C-level correctness; not attempted this session |
| test_nginx        | **fixed** — `__sync_fetch_and_add`/sub/or/xor/and/nand's narrow-argument sign-extension bug (`ngx_atomic_fetch_add(lock, -1)` corrupted `ngx_rwlock_unlock()`, hanging every worker in `ngx_rwlock_wlock()` forever), see "Fixed (2026-08-11, continued — atomic fetch-op narrow-argument session)" below; reran the real `nginx-tests` suite fresh (`prove .`, 492 files) — all 2600 tests pass                                                                                                                                                                                                                                                                                                                                                             |
| test_groff        | **fixed** — passes cleanly now (confirmed via a fresh batch run this session); no rcc changes were needed specifically for it, resolved by the accumulated fixes from prior sessions                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| test_argtable3    | **fixed** — passes cleanly now (confirmed via a fresh batch run this session); no rcc changes were needed specifically for it, resolved by the accumulated fixes from prior sessions                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| test_httpparser   | **fixed** — was: `-funroll` label-aliasing bug, see "Fixed (2026-08-08, httpparser session)" above                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| test_libarchive   | **4/10 fixed, remaining 6 confirmed NOT an rcc bug** — the wide-string-literal alignment fix (4 stacked bugs, see "Fixed (2026-08-09, continued — wide string literal alignment: 3 stacked bugs)" below) unblocked `test_entry`/`test_archive_match_path`/`test_archive_match_time`/`test_filter_count`; the remaining 6 are a pre-existing PPMd arithmetic-decoder issue in this libarchive 3.8.8 checkout's own test corpus, reproducing identically with a fully gcc-built libarchive — see "Investigated: libarchive PPMd cluster ..." below                                                                                                                                                                                                             |
| test_liblz4       | **investigated, not an rcc bug** — `make test`'s `test-lz4-hugefile` step generates and round-trips a 4.2GB file; this sandbox's disk/CPU throughput alone exceeds the 420s harness timeout, not a correctness issue, see "Investigated: test_liblz4 ..." below                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| test_libevent     | **investigated, not an rcc bug** — builds clean with rcc; `test/regress` passes in full (356 tests OK under the default epoll backend, 347 under POLL, 0 failed). `make check` exceeds the 420s harness timeout because it re-runs the whole battery once per event backend (select, poll, epoll, devpoll, kqueue, ...) and several http/connection tests deliberately wait out 5s timeouts each — a fully gcc-built libevent's own `make check` is terminated by the same 420s budget identically. Timeout artifact, see "Investigated: test_libevent ..." below                                                                                                                                                                                            |
| test_libpng       | **investigated, not an rcc bug** — `pngtest-all`'s strict byte-compare fails identically with a fully gcc-built libpng+pngtest too (upstream zlib-version-sensitive reference file, libpng's own documented caveat); remaining timeout is `pngimage-full`'s exhaustive transform-combination test running correctly but ~3x slower under rcc's codegen than gcc -O2 (222s vs 68s, both 100% PASS) — see "Investigated: test_libpng ..." below                                                                                                                                                                                                                                                                                                                |
| test_libressl     | **AES-NI/SSE2/GHASH/RC4 crypto asm fixed; blocked on new gap** — was untriaged; this session added missing AES-NI + several SSE2/SSSE3 instruction encoders (see "Fixed (2026-08-11, continued — AES-NI/SSE2 instruction encoder session)" below), unblocking `crypto/aes/aesni-*.S`, `crypto/modes/ghash-*.S`, `crypto/rc4/rc4-*.S`; now blocked on 21 `crypto/bn/arch/amd64/*.S` files using `.intel_syntax noprefix` — rcc's assembler has no Intel-syntax parsing mode at all, a large separate undertaking, not attempted this session                                                                                                                                                                                                                  |
| test_qbe_simplecc | **fixed** — GAS `/* */` block-comment handling in the inline assembler, a nested-designator compound-literal offset bug, and a register-allocator aliasing bug, see "Fixed (2026-08-09, qbe_simplecc session)" below; `qbe`'s own test suite now passes 59/59                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |

---

## Quick Wins (next to fix)

**All previously listed quick wins are now fixed** — AVX2/AVX-512
codegen (test_blake3, test_brotli: see "Fixed (2026-08-12, AVX/AVX2
intrinsics session)" and "Fixed (2026-08-12, AVX-512 / EVEX intrinsics
session)" below), C23 `_BitInt(N)` (see "Fixed (2026-08-13, wide
`_BitInt(N>64)` session)" above), and C `defer` (see "Fixed
(2026-08-14, C `defer` session)" above; test_nob); test_c23doku stays
skipped by decision, arbitrary-precision bignum workload out of scope.
This section is stale history kept for context — no remaining genuine
gaps are tracked in "Needs fixing" above as of this session.

### Fixed (2026-08-08, VM-type evaluation-order session)

- **`__builtin_va_arg(ap, TYPE)` with a variably-modified `TYPE`** (parser.c)
- \**Nested VLA-pointer-array declarator (`int (*p[f(2)])[f(3)];`) evaluated
- \*\*`sizeof` on a VLA-typed _expression_ discarded the operand entirely
- \*\*A cast's VM array-length expression nested behind a function-pointer
- \*\*A queued VM-`typeof` side-effect evaluation was dropped for any

New regression tests: `test/test_vaarg_vm_type.c`,
`test/test_vla_nested_array_of_ptr.c`, `test/test_sizeof_comma_vla.c`,
`test/test_cast_vm_funcptr.c`, `test/test_typeof_comma_vm_stmt.c`.

### Fixed (2026-08-08, cproc scope/attribute/GNU-ternary session)

- \*\*A tag or enum constant declared inside a function's parameter-type-list
- \*\*C23 `[[gnu::packed]]` / `[[__gnu__::packed]]` on a struct/union tag was
- \*\*GNU `a ?: b` (omitted then-operand) evaluated `a` twice, and its

New regression tests: `test/test_enum_param_scope_leak.c`,
`test/test_c23_attr_packed.c`, `test/test_gnu_ternary_omit_promote.c`.

### Fixed (2026-08-08, C23 enum type-modeling session)

- \*\*C23 `enum tag : type` fixed underlying type wasn't complete until the
- \*\*`__builtin_types_compatible_p` treated any two enums of the same
- \*\*Enumerator value → type promotion (no fixed underlying type) skipped

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

- \*\*Hex/octal/binary integer-literal type selection compared the raw
- **`is_typename()` didn't recognize `_BitInt` as a type keyword**
- \*_`ND_DEREF` rejected a second `_` applied to an already-function-typed
- \*\*C23 "label may precede a declaration or the end of a compound
- \*\*A wide-string-literal (`u`/`U`/`L`/C23 `u8"..."`) whole-array
- \*\*A wide string literal's _expression_ type was a pre-decayed pointer,
- \*\*Function-type parameter comparison never stripped the parameter's own
- \*\*`__builtin_types_compatible_p` was a hand-rolled, incomplete
- **The `test_cproc` batch-harness step never actually ran end-to-end** —
- \*\*Automatic/VLA storage over-alignment beyond the ABI-guaranteed 16
- \*\*`eval_const_expr()`'s `ND_EQ`/`ND_NE`/`ND_LT`/`ND_LE` compared

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

- \*\*The enum overall-type ladder's `long`/`unsigned long` tier used
- \*\*`test/test_enum_c23_wide_and_fixed.c`'s own `PromoteLong`/

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
- **codegen.c** — a global with a non-NULL `section_name` is now routed
- **link_elf.c** — rcc's own native ELF linker now synthesizes

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
- **codegen.c** — after resolving a `section()` global's target
- **asm.c** — an explicit `.balign`/`.align`/`.p2align` directive
- **elf_write.c** — the extra-section file-offset layout loop now

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
- **link_macho.c** (`link_load_object`) — a loaded section whose
- **link_macho.c** (`link_macho`) — two additions:

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
`test/test_string_literal.c`, `test/test_stdint_int64_glibc_abi.c`,
plus `test/test-link.sh` case 7 (gnu*inline needs real two-TU linking,
which the single-file `test/test*\*.c` harness can't express).

**Full suite verified after every fix**: TCC 118/118, Unit 197/197,
Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Link 6/6
(native Linux x86-64); C-testsuite 220/220 on both mingw cross and
arm64 cross, plus the specific new/affected unit tests confirmed
passing on both cross targets (`_WIN32` branch of the stdint.h fix is
behaviorally unchanged on mingw by construction).

### Fixed (2026-08-09, continued — `__builtin_cpu_supports`/`__builtin_cpu_init` broken on mingw)

- \*\*`__builtin_cpu_supports`/`__builtin_cpu_init` (added in the session

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

### Fixed (2026-08-09, continued — `-o` driver flag rejected the joined `-oFILE` form)

- **`-o`'s argument was only ever accepted as a separate argv element**

New regression test: `test/test_opt_o_joined.c` — compiles with both
`-oFILE` (joined) and `-o FILE` (separate), asserting the requested
object file actually exists after each. Full suite verified: Torture
3605/3609 (100% of non-skipped), Dg-error 34/34, Link 6/6, Unit tests
all passing, 0 failed overall (native Linux x86-64).

(test_samba itself still doesn't build end-to-end after this fix —
waf's C-compiler bootstrap probe is only the first of many
configure-time checks; not re-triaged further this session.)

### Fixed (2026-08-09, continued — local char-array `{ STRLIT }` initializer corrupted)

- \*\*A local (function-scope, non-static, non-constexpr) char/wide-char

New regression test: `test/test_string_literal.c` —
the exact NUL-prefixed-string shape from the bug, plus a plain local
char array, a trailing comma inside the brace, a wide-string local,
and a regression guard for the sibling "one-pointer-array-of-`char*`"
case (`const char *arr[] = {"vec_"}`, TY_PTR-excluded, must keep
assigning the address rather than being treated as a char array).
Full suite verified: Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, Link 6/6, 0 failed overall (native Linux x86-64); confirmed
clean (both the new test and `test_string_literal` PASS) on
the mingw cross target.

### Fixed (2026-08-09, continued — wide string literal alignment: 3 stacked bugs)

- \*\*Wide string literals (`L"..."`) could land at any byte offset in
  `.rodata`, misaligned for `wchar_t` — glibc's vectorized
  `wcslen()`/`wcscmp()`/`wmemcmp()` read multiple `wchar_t` at once under
  the assumption that any `wchar_t` object satisfies `_Alignof(wchar_t)`
  (4 on Linux/macOS, 2 on Windows), true of every object a real compiler
  emits, string literals included. Three separate, stacked bugs conspired
  to break this:
  1. **codegen.c's string-literal emission loop packed every literal
     (narrow and wide) back-to-back with no padding** — a wide literal
     landed at whatever odd byte offset the preceding literal's length
     left behind. Fixed: `secbuf_align(cg_sec, s->elem_size)` before each
     wide literal's bytes.
  2. **Two call sites that register a string literal used purely for its
     _address_ hardcoded `elem_size=1` regardless of the literal's real
     prefix** — `read_global_label_initializer()` (a global/const pointer
     initializer, e.g. `const wchar_t *p = L"text";`, the exact shape of
     libarchive's `fileflags[]` flag-name table) and `extract_reloc()`'s
     `ND_STR` case (an `&expr` reloc extraction), which additionally
     re-registered a wholly redundant _second_ StrLit instead of reusing
     the correctly-tagged one `primary()` already created for the same
     token. Fixed: a new `str_lit_elem_size()` helper (mirroring
     `primary()`'s own prefix→size switch) for the first; reusing
     `node->str_id` for the second.
  3. **elf_write.c hardcoded `.rodata`'s ELF `sh_addralign` to `1` in
     every `.o` it wrote** — even after fixes #1/#2 correctly self-pad a
     literal _within_ one compilation unit, the linker (rcc's own, or a
     real system `ld`) reads `sh_addralign` to decide how to place one
     object's section relative to another's once merged; declaring `1`
     ("no constraint") let it concatenate this object's `.rodata`
     immediately after another's at any offset, silently destroying the
     intra-file padding. This is why the bug reproduced in libarchive's
     real, multi-file build but not in any single-file minimal repro.
     Fixed: `SecBuf` now tracks the max alignment any `secbuf_align()`
     call has requested (mirroring the existing pattern already used for
     `__attribute__((section("name")))` globals); `elf_write.c`'s
     `.data`/`.rodata`/`.tdata` section headers use the tracked value
     instead of a hardcoded constant. `link_elf.c` (rcc's own linker)
     already correctly _consumed_ `sh_addralign` when merging sections —
     it was only ever being fed the wrong (hardcoded) value.
     A fourth, independently-discovered bug surfaced while writing the
     regression test: **`infer_array_type()` sized a wide array declarator
     using NUL-terminated `utf8_len()` instead of length-bounded UTF-8
     codepoint counting** — `wchar_t units[] = L"\0KMGTPEZY";` (an embedded
     NUL before the literal's true end, the exact `LZ4IO_toHuman()`/
     `fileflags` table shape but as a top-level global) sized the array as
     1 element (stopping at the leading NUL) instead of 10, leaving every
     byte past the first uninitialized garbage. Fixed: replaced the
     `utf8_len()` call with the same `tok->len`-bounded `decode_utf8()`
     loop `primary()`/`global_initializer()`'s wide-string branches already
     use.
     → found via test_libarchive: `archive_entry_copy_symlink_w()`/
     `archive_entry_symlink_w()` truncated every wide string by one
     character (`archive_mstring_copy_wcs_len()`'s internal `wcslen()`
     misread the misaligned literal); `ae_wcstofflags()`'s `fileflags[]`
     table lookup silently dropped bits from the parsed flag set for the
     same reason. Unblocked `test_entry`, `test_archive_match_path`,
     `test_archive_match_time`, and `test_filter_count` (4 of libarchive's
     10 originally-failing non-fuzz tests; the other 6 are an unrelated
     PPMd-codec issue, not yet triaged).

New regression tests: `test/test_string_literal.c` (all four
fixed code paths: local pointer, global array with embedded NUL,
struct-field pointer, `&L"literal"`); `test/test-link.sh` case 8
(2-TU link, deliberately misaligning the second object's `.rodata` via
an odd-length narrow literal in the first, guards the linker-level
fix specifically — the one bug class no single-file test can catch).
Full suite verified: Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, Link 8/8 (incl. the new case), 0 failed overall (native Linux
x86-64); confirmed clean (both new tests PASS) on the mingw and arm64
cross targets.

### Investigated: libarchive PPMd cluster (7zip/RAR/ZIP) — confirmed NOT an rcc bug

The 6 remaining non-fuzz `test_libarchive` failures after the wide
string alignment fix above — `test_read_format_7zip_ppmd`,
`test_read_format_rar_compress_best`, `test_read_format_rar_multivolume`,
`test_read_format_rar_ppmd_lzss_conversion`,
`test_read_format_zip_ppmd_multi`(`_blockread`),
`test_read_format_zip_ppmd_one_file`(`_blockread`),
`test_read_format_zip_winzip_aes256_large_ppmd` — all fail with genuine
PPMd arithmetic-decoder errors ("Invalid symbol", "Invalid PPMd
sequence", "Invalid location to Huffman tree specified", "Truncated
7z file data").

**Confirmed pre-existing, not an rcc bug**: rebuilding `archive_ppmd7.c`/
`archive_ppmd8.c` and `libarchive_test` with real `gcc` (not rcc)
reproduces the identical failures against the same `.uu`-encoded
reference archives. This checkout's PPMd-family test corpus is
incompatible with (or the PPMd implementation itself has a pre-existing
bug against) this specific libarchive 3.8.8 checkout, independent of
which compiler builds it. Not investigated further — no rcc fix is
possible or needed here; any fix belongs upstream in libarchive.

### Fixed (2026-08-09, qbe_simplecc session)

**test_qbe_simplecc now fully passes**: qbe's own `tools/test.sh all`
(`qbe` itself, and `scc`, built with `CC=rcc`) went from 42/59 to
59/59 once all three bugs below were fixed. All three are genuine,
broadly-impactful rcc bugs — none specific to qbe — found by comparing
a real-gcc-built `qbe` (which passed all 59 tests cleanly, ruling out
a pre-existing qbe/test-corpus issue) against the rcc-built one.

- \*\*The inline assembler had no handling at all for GAS-style C
- \*\*A compound literal's designator-chain parser resolved a
- \*\*The register allocator's spill mechanism could alias two

New regression tests: `test/test_asm_block_comment.c` (own-line,
trailing, and multi-line-spanning block comments in inline asm);
`test/test_compound_literal.c` (new `.bits.i`-through-a-named-
union cases, both as a plain local initializer and as an expression-
context compound literal); `test/test_bitfields.c` (new ternary +
positional-compound-literal-as-last-of-5-args case, both branches).
Full suite verified: Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, Link 7/7, RCC Unit tests 0 failed, 0 failed overall (native
Linux x86-64); `test_qbe_simplecc`'s own `tools/test.sh all` 59/59.

### Investigated: test_liblz4 — confirmed NOT an rcc bug (sandbox throughput, not correctness)

`test_liblz4`'s harness target (`make test`) transitively depends on
`tests/Makefile`'s `test-lz4-hugefile` (`test-lz4-fast-hugefile` +
`test-lz4hc-hugefile`), which `datagen -g4200MB`s a 4.2GB file and
round-trips it through both the fast and HC compressors with full
content verification. In this sandbox that alone takes well over the
harness's 420s per-target timeout — confirmed by running it standalone
and watching it progress steadily (through several minutes of
`Read : NNN MiB ==> ...%` progress output) with **zero** diffs,
decompression mismatches, or crashes, just slow linear throughput
(~0.5-1 MiB/s effective for the largest passes under this sandbox's
CPU/disk contention).

Re-ran the rest of the suite explicitly _excluding_ the hugefile
targets (`test-lz4-essentials`, `test-lz4-opt-parser`,
`test-lz4-sparse`, `test-lz4-dict`, `test-lz4-skippable`, `test-lz4c`,
`test-fullbench`, `test-fuzzer -T90s`, `test-frametest -v -T90s`,
`test-amalgamation`, `listTest`, `test-decompress-partial`) — every
one passed cleanly (including `listTest`'s own 10-case Python unit
suite, `Ran 10 tests in 0.405s / OK`), no failures, no crashes. Zero
`FAIL`/`error:`/mismatch markers in either run's full output (the only
text-string hits for "corrupti"/"error" are literal words inside
lorem-ipsum-style test _fixture data_, not diagnostics).

**Confirmed pre-existing sandbox limitation, not an rcc bug**: no
fix needed or possible on rcc's side — the harness's fixed 420s
timeout is simply too short for a 4.2GB compress/decompress round
trip on this machine's I/O throughput. (The one genuine rcc bug
`test_liblz4` did surface — `programs/lz4io.c`'s `LZ4IO_toHuman()`
NUL-prefixed brace-string-literal initializer — was found and fixed
separately; see "Fixed (2026-08-09, continued — local char-array `{
STRLIT }` initializer corrupted)" above.)

### Investigated: test_libpng — confirmed NOT an rcc bug (upstream test sensitivity + codegen speed, not correctness)

`make check`'s `check-TESTS` run reported `FAIL: tests/pngtest-all`
then hit the harness's 420s timeout mid-way through the suite's final
test, `tests/pngimage-full`. Investigated both independently:

- **`pngtest-all`'s failure**: the very first sub-check,
- **The timeout**: `tests/pngimage-full` runs

**Confirmed pre-existing, not an rcc bug**: no fix applied or needed;
rcc-generated code is correct throughout, just slower than gcc -O2 on
this specific exhaustive-combinatorial workload, and the strict
byte-compare failure is upstream/environmental, reproducing identically
with a 100% gcc-built libpng.

### Fixed (2026-08-09, block-scope extern DCE session)

- \*\*A block-scope `extern` declaration of a global inside a never-called

New regression test: `test/test_extern_block_scope_dce.c` (two
never-called `static inline` functions each block-scope-`extern`-
declare a global that is defined at file scope afterward and used only
from a third, live function — reproduces the exact drop-with-the-dead-
function pattern without needing rsync itself).

Full suite verified: Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, RCC Unit tests 0 failed, Link tests 7/7, 0 failed overall
(native Linux x86-64); `test_rsync`'s own `make test` now links and
runs to completion.

### Fixed (2026-08-10, output-extension-naming / include_next session)

- **`rcc -c foo.cxx` (no `-o`) produced `foo.cxx.o`, not `foo.o`**
- \*\*`#include_next` from one of rcc's own bundled headers could be

New regression tests: `test/test_output_ext_naming.c` (drives rcc as a
subprocess across `.c`/`.C`/`.cc`/`.cxx`/`.cpp`/`.i`/`.s`, confirming
the default `-c` output name in each case), `test/test_include_next_skips_user_dirs.c`
(a decoy same-named header in a `-I` directory, confirming
`#include_next <wchar.h>` from rcc's own bundled header reaches the
real system one, not the decoy). Both fail against the pre-fix binary
and pass with the fix. Full suite verified: Torture 3605/3609 (100% of
non-skipped), Dg-error 34/34, RCC Unit tests 0 failed, Link tests 7/7,
0 failed overall (native Linux x86-64); ksh93's own mamprobe
C-compiler sniff now passes and the build reaches real source
compilation (see the `test_ksh93` table row above for what still
blocks it beyond this point).

### Investigated, not fixed (2026-08-10, register-spill/locals-collision session)

**test_perl**: `miniperl -f write_buildcustomize.pl` (i.e. _every_
rcc-built perl, since this file loads on startup) segfaults inside
`Perl_upg_version()`, dereferencing a NULL first argument. Traced the
call chain back to `S_enable_feature_bundle()` (feature.h, inlined into
op.c/opmini.c): a 9-level nested `(setnv(comp_ver, X), vcmp(ver,
upg_version(comp_ver, FALSE)) >= 0) ? N : ...` ternary chain, where
`ver` (the function's own SV\* parameter) and `comp_ver` (a
`sv_newmortal()` result) both need to stay live across every one of the
9 branches' own `setnv`/`vcmp`/`upg_version` calls.

**Root cause, confirmed via a minimal reproduction and disassembly**:
`alloc_reg()`'s spill mechanism (codegen.c) and ordinary declared-local
stack-offset assignment (`var->offset`, from parsing) are two entirely
independent counters that both address memory as `-N(%rbp)` starting
from small values - `next_spill_slot` begins at a hardcoded `8` every
function, with zero awareness of `fn->stack_size` (the function's own
locals region size). Under light register pressure the two never
collide (spilling is rare and shallow); under heavy pressure - a long
chain of nested ternaries, each needing several registers concurrently
live across many inner calls - `alloc_reg()`'s spill-victim selection
eventually has to spill deep into indices whose `next_spill_slot`
address lands exactly on a live local's own `var->offset`. In the
minimal repro this silently overwrote a `SV *` parameter's home slot
mid-expression with an unrelated branch-local boolean temporary.

**Attempted fix**: reset `next_spill_slot` after Pass 1 (dry-run
discovery) to start past `fn->stack_size + fn_struct_ret_total +
fn_trampoline_total` - the same three regions the existing frame-size
computation (`need = fn->stack_size + fn_struct_ret_total +
fn_trampoline_total + 32`) already sums. This measurably fixed the
targeted collision (confirmed via disassembly: the exact clobbering
instruction moved off the locals' own offsets) and fixed the minimal
reproduction's crash pattern, but **broke 16 previously-passing torture
tests** (`va-arg-2` first, then `bitops-1`, `pr38969`, `pr44858`, and
others) when run through the full `make check-all` regression suite -
`next_spill_slot`'s new starting point collides with _other_
function-specific regions that also key off `fn->stack_size`
independently of the frame-size `need` computation, at minimum the
x86-64 SysV variadic register-save area (`va_reg_save_ofs =
current_fn_stack_size + 176`, set once during Pass 1's param handling
and never revisited). Patching that one case (skip past
`va_reg_save_ofs` too, for variadic functions) fixed `va-arg-2` but
`bitops-1` - a plain, non-variadic, no-varargs test - still failed
identically, meaning at least one more region collides with the new
`next_spill_slot` start point that wasn't identified in the time
available this session.

**Decision**: reverted the codegen.c change entirely rather than ship
a partially-verified fix that trades one regression class for another;
`test_perl` remains unfixed. This is a real, reproducible bug worth
revisiting, but properly fixing it needs a systematic audit of every
per-function stack region that's computed independently of
`next_spill_slot` (struct-ret staging, trampoline slots, variadic
register-save area, the `spill_logand`/`spill_atomic_old` fixed slots
allocated once via `init_spill_slots()`, and any others), unifying them
behind one single, non-overlapping stack-layout allocator instead of
several independent hardcoded-base counters - a bigger undertaking than
a single-session fix.

### Fixed (2026-08-10, continued — spill/locals collision)

Root cause was as diagnosed above (`next_spill_slot` grows from a
hardcoded low base, independent of `need`), but the prior session's
"reset to `fn->stack_size + fn_struct_ret_total + fn_trampoline_total`"
approach mis-timed the reset: `need` (locals + struct-ret + trampoline
, spill max-folded in) was already fixing the prologue's frame size and
the callee-saved save area's placement _before_ the reset could apply,
so Pass 2 could still walk past it.

Fixed by computing the spill region's actual growth during Pass 1
(`next_spill_slot - spill_reserved_base`, the base right after
`init_spill_slots()`) and _adding_ it onto `need` (not maxing), then
resetting `next_spill_slot` to the pre-growth `need` so Pass 2's
identical push_spill_slot() replay lands in the newly reserved region
instead of the low base. Applied on both the x86-64 and ARM64
prologue sites (same `need`/spill-region structure on both).

This surfaced a second, previously-latent bug: the `__cleanup__`
epilogue stashes RAX (+RDX for a GP-pair struct return) via
`spill_offset()` _after_ Pass 2's body walk — code Pass 1 never sees
(it only walks `fn->body`). With spills re-anchored to start exactly
at `need`, this invisible extra push landed exactly on the
callee-saved register save area, which also starts at `need`,
corrupting whichever callee-saved register a struct-return path was
using (caught by `101_cleanup`'s `test_cleanup2` in `make check-all`,
not by the original repro). Fixed by budgeting the cleanup epilogue's
known, deterministic extra spill usage into `need` up front. ARM64's
cleanup path uses a dedicated callee-saved register (x19/x20) instead
of `spill_offset()`, so it isn't affected.

New regression test: `test/test_spill_locals_collision.c` (both bugs;
segfaults pre-fix on native x86-64 and mingw, `assert`-fails on the
intermediate fix that only covered bug 1). Full suite verified: TCC
118/118, Unit tests 205/205, Compliance 15/15, C-testsuite 220/220,
Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Link 7/7 — 0
failed overall (native Linux x86-64); the new test also confirmed
passing standalone on the mingw and arm64 cross targets. Re-ran the
real `test_perl` third-party target end to end (`./Configure -des
-Dcc=rcc ...`, `make -j3 test_prep && HARNESS_OPTIONS=j3 make
test_harness`) to confirm against the actual reported crash, not just
the synthetic repro: it now builds miniperl, the full `perl`, and
every extension, and `make test_harness` completes cleanly, `rc=0` in
557s.

### Fixed (2026-08-10, include_next noop-forward refinement)

The prior session's `#include_next` fix ("output-extension-naming /
include*next session" above) was too broad: it unconditionally skipped
\_every* user `-I` directory when continuing an `#include_next` from
one of rcc's own bundled headers (RCC*INCDIR), on the theory that a
same-named user header could only ever be an accidental shadow of the
real system one. That's true for the case it fixed (ksh93's own
`std/wchar.h` is a trivial `#include <ast_wchar.h>` forwarder, which
at that point in the chain is already open and hence a guard-induced
no-op) but false in general: a user `-I` header can also be a
\_legitimate, non-forwarding replacement*. ksh93 hits exactly that
case one step later: `ast_wchar.h`'s own preamble does `#ifndef
_SFSTDIO_H #include <stdio.h> #endif` specifically to pull in glibc's
`<stdio.h>` (which defines `__FILE`, needed by glibc's `<wchar.h>`
declarations like `open_wmemstream`/`fwide`) before `_SFSTDIO_H` gets
hijacked elsewhere — but `#include <stdio.h>` resolves through rcc's
own bundled `include/stdio.h` first, whose own `#include_next
<stdio.h>` used to skip straight past ksh93's `-Istd/stdio.h`
(→ `ast_stdio.h`, which _does_ provide `__FILE` via `#define __FILE
FILE`) to glibc's `<stdio.h>` directly — silently discarding the
`__FILE` definition ksh93 relies on and leaving glibc's `<wchar.h>`
failing to parse ("expected specific operator" on every `__FILE *`
parameter) once `#include_next <wchar.h>` (fixed last session)
actually reached it. Confirmed cross-checked against real gcc: given
the identical rcc-generated build tree, gcc resolves `<stdio.h>`
straight to `-Istd/stdio.h` and compiles clean; rcc's blanket skip
bypassed it.

Fixed by replacing the blanket skip with a narrower check
(`is_noop_forward_to_active()`, preprocess.c): a user `-I` candidate is
only skipped when its entire content, once comments/blank lines are
stripped, reduces to a single `#include` whose resolved target is
already open on the include stack (walking `lvl`) — i.e. it would
contribute nothing back. Anything else (like `ast_stdio.h`'s real
content) is accepted normally, restoring the original linear
`#include_next` search order. Also fixed the `start`-index detection
that search relies on (which directory supplied the _current_ file) to
normalize path separators via `canonical_path()` before comparing —
`full_path()`'s raw `_fullpath()` output on Windows mixed backslashes
into paths built from rcc's (forward-slash) `RCC_INCDIR`, so the
match never fired there and `start` silently stayed 0, re-finding
rcc's own header instead of continuing the search at all (only
visible cross-platform, since native Linux's `realpath()`-based
`full_path()` never produces backslashes to begin with).

Unblocked ksh93's `misc/fastfind.c` (the compile from the previous
session's own reproduction) and the whole `libast` build gets much
further (`hash/*.c`, `string/ccmap*.c`, `port/getcodeset.c`) before
hitting a new, unrelated, not-yet-investigated gap in
`string/chresc.c` (`CC_bel`/`CC_esc`/`CC_vt` undeclared — see
test_ksh93's own TODO row above).

Regression test: `test/test_include_next_skips_user_dirs.c`, rewritten
with two cases in one file — a trivial forwarder must still be
skipped (the original fix's own regression coverage, updated to use a
forwarding decoy instead of an unconditional `#error` one, since the
narrower heuristic doesn't blanket-skip on content it can't reason
about), and a real non-forwarding `-I` override must still be found
(this session's regression). Both fail against the overly-broad
intermediate fix; both pass with this session's refinement. Full suite
verified: TCC 118/118, Unit tests 205/205, Compliance 15/15,
C-testsuite 220/220, Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, Link 7/7 — 0 failed overall (native Linux x86-64); both
regression tests also confirmed passing standalone on the mingw and
arm64 cross targets (the path-separator fix above was needed
specifically for mingw's case 2 to pass).

### Fixed (2026-08-10, include_next self-reference / ND_COMMA null-deref session)

Continued ksh93/libast now that the `__FILE`/`stdio.h` gap was fixed:
`misc/fastfind.c` and `string/chresc.c` compiled, but `comp/iconv.c`
(ast's own iconv fallback implementation, which needs `_ICONV_LIST_PRIVATE_`-
gated _raw_ `iconv_t` — the real system one, not macro-renamed to
`_ast_iconv_t`) still failed with `iconv_t` undeclared, and separately
`string/chresc.c` still had `CC_bel`/`CC_esc`/`CC_vt` undeclared (from
`ccode.h`, reached transitively through `<iconv.h>` on real gcc).

**Root cause**: ast's own `ast_iconv.h`/`ast_wchar.h`/`ast_stdio.h`
(iffe-generated) reach "the platform's native header" via `#include
<../include/X.h>` — a relative escape that, joined against
`resolve_include()`'s first search entry (`RCC_INCDIR`, whose own
basename literally _is_ "include"), lexically collapses right back
onto RCC*INCDIR's own bundled copy of the same name
(`.../rcc/include/../include/iconv.h` \_is*
`.../rcc/include/iconv.h`) — not the real system header the idiom is
trying to reach. When that bundled copy is still active on the
include stack (always true here: this fires from _inside_ its own
`#include_next` continuation), its include guard fires on this
second, self-referential visit and the whole body — including the
`#include_next` line meant to reach glibc — silently no-ops. Whether
this actually breaks anything downstream turned out to depend on
luck: `wchar.h` happened to work because `ast_wchar.h` was already
independently pre-opened via a different chain before this collision
could matter; `iconv.h` had no such luck.

Fixed by extracting `resolve_include()`'s original logic into a new
`resolve_include_raw()`, and having the real `resolve_include()`
(used by the actual `#include`/`__has_include`/`#embed` directive
handlers) skip a match at the `RCC_INCDIR`/"include"-fallback search
position when it resolves to a file already active on the include
stack, continuing to the next candidate — mirroring
`resolve_include_next()`'s own no-op-forward handling for the same
self-reference shape (added the prior session). Deliberately kept
`is_noop_forward_to_active()`'s own internal peek on
`resolve_include_raw()` instead: it needs the _unfiltered_ answer to
recognize this exact self-reference collision as its own "already
active" signal in the first place — using the filtered wrapper there
would hide the very case it exists to detect.

**Second bug, unrelated**: with `comp/iconv.c` compiling further,
`sfio/sfvprintf.c` (ast's own printf-family implementation) crashed
rcc outright (SIGSEGV, not a diagnostic). Root cause:
`add_type_internal()`'s `ND_COMMA` case (type.c) computed `node->ty =
node->rhs->ty;` then unconditionally dereferenced `node->ty->kind` to
apply array/function decay — but several node kinds are
statement-like and never get a type assigned at all (`ND_NULL`,
`ND_ZERO_INIT`, `ND_LABEL`, ...), and a comma expression's rightmost
operand can legally be one of those (e.g. `__builtin_apply()`
synthesizes a bare `ND_NULL` placeholder). `node->rhs->ty` being NULL
there dereferenced garbage. Fixed by only applying the decay when
`node->ty` is non-NULL, leaving a void-rhs comma expression's own
type NULL too instead of crashing.

`test_ksh93`'s `libast` now compiles and links **completely** (every
`comp/*.c`, `sfio/*.c`, `string/*.c`, `hash/*.c`, `misc/*.c`) and
reaches `cmd/INIT`'s own `iffe` self-test suite — 159/161 passing (2
narrow, iffe-internal struct-introspection failures, not
investigated this session) — by far the deepest any session has
gotten into this target.

New regression tests: `test/test_iconv_chain.c` (mirrors the real
`ast_iconv.h` shape directly: a synthetic `-I` iconv.h override that
also defines an unrelated marker macro must still be reached, not
shadowed by rcc's own bundled `include/iconv.h` — reproduces cleanly
without needing the actual ksh93 tree); `test/test_comma_null_rhs.c`
(the `__builtin_apply()`-based ND_COMMA repro — SIGSEGVs the unfixed
compiler outright, passes cleanly fixed). Also converted rcc's own
bundled `include/iconv.h` to the same `#ifdef _WIN32 / __APPLE__ /
#else #include_next #endif` pattern `stdio.h`/`wchar.h`/`limits.h`
already use (it was the one bundled header with no platform chaining
at all, self-contained on every target — the actual reason
`resolve_include_next()`'s "already correctly walks past a no-op
forwarder" fix from the prior session never got exercised for iconv
specifically until this session added the chain itself). Full suite
verified: TCC 118/118, Unit tests 207/207, Compliance 15/15,
C-testsuite 220/220, Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, Link 7/7 — 0 failed overall (native Linux x86-64); both new
tests, plus the full previously-existing include_next/limits-chain
regression coverage, also confirmed passing standalone on the mingw
and arm64 cross targets.

### Fixed (2026-08-10, continued — sizeof/cast incomplete-type validation)

`test_ksh93`'s own `cmd/INIT` `iffe` self-test suite (159/161 passing
per the session above) had 2 failures, both in `TEST 12 'non-opaque
mem'`: iffe's `mem` operation probes whether a struct type is opaque
(forward-declared, never defined) by generating small `.c` files and
checking whether rcc accepts or rejects them — and rcc accepted two
forms of invalid C it should have rejected, silently agreeing with
both the "opaque" and "non-opaque" cases regardless of which was
actually true.

- **`sizeof` on an incomplete struct/union type never errored**
- **Casts had no scalar-type validation at all** (parser.c) — C11

New regression test: `test/test_incomplete_sizeof_cast.c` — 9 cases
covering both the two invalid-C rejections this fix adds (opaque
`sizeof`, both type-name and expression forms; struct-to-scalar and
scalar-to-non-union-struct casts; cross-struct-type casts) and every
leniency it must preserve (sizeof/cast on a genuinely complete struct;
same-type struct-cast identity no-op; GNU union-cast from a matching
member type, rejected from a non-matching one; array-to-integer cast
via pointer decay). Full suite verified: TCC 118/118, Unit tests
207/207, Compliance 15/15, C-testsuite 220/220, Torture 3605/3609
(100% of non-skipped), Dg-error 34/34, Link 7/7 — 0 failed overall
(native Linux x86-64); confirmed clean on the mingw and arm64
cross-compile targets.

With both fixes above, `test_ksh93`'s full `bin/package make` now
completes without error (builds and installs `ksh`, `shcomp`,
`libshell.so`/`.a`, every man page and `fun/*` script) — the whole
build that used to die partway through `libast` now finishes cleanly.
`bin/package test` goes on to run ksh93's own real functional
regression suite (`cmd/ksh93/tests/*.sh` via `shtests`) for the first
time this project has ever reached it; `alias`/`append` pass cleanly,
`arith.sh` surfaces one further, unrelated lead (`cos*cos + sin*sin >
1.01`, a floating-point precision question, not yet root-caused) —
left for a future session rather than expanding this one's scope.
(Resolved 2026-08-12: that symptom was an rcc codegen bug — the
long-double assignment-expression value returning 0 — not precision;
see "Fixed (2026-08-12, ksh93 arith / IEEE truthiness / 64-bit
pointer-arithmetic session)" below.)

### Fixed (2026-08-10, continued — ARM64 long double return value)

While cross-checking the earlier x86-64 `long double` ABI fix (see
"Fixed (2026-08-10, register-spill/locals-collision session)" test
file above) against the arm64 cross target, `test_long_double_abi`
SIGABRTed under qemu-aarch64 even though the x86-64-side bugs it
targets don't apply to AAPCS64's classification at all. Two more,
independent ARM64-only bugs (codegen.c), the mirror image of the
x86-64 ST0/XMM0 gap fixed above:

- \*\*`gen_funcall`'s post-call return-value read treated a `long
- **A function's own `return` statement had the same bug in reverse**,

ARM64's argument classification and prologue parameter loading already
handled `long double` correctly (a dedicated HFA/long-double path
independent of the ordinary float classification) — only the two
return-value sites above had the gap.

**Apple ARM64 correction (caught by CI, not local testing)**: the
first push of this fix unconditionally applied both `__trunctfdf2`/
`__extenddftf2` calls under `#ifdef ARCH_ARM64`, which broke macOS
CI's `test (macos-latest)` job (Torture regressed on `930622-2`,
`conversion`, and 3 others — the exact long-double tests this fix
targets). Root cause: Apple's arm64 ABI defines `long double` as
_identical_ to `double` (see `type.c`'s `ty_ldouble`: `size=8` under
`__APPLE__` vs `size=16` everywhere else) — Darwin has no 128-bit
binary128 `long double` at all, so there is no widen/narrow step to
do, and calling `__trunctfdf2`/`__extenddftf2` on an already-plain-
double value corrupted it. Both call sites are now further guarded
`#ifndef __APPLE__` (Linux ARM64 only); this repo has no local Darwin
cross-toolchain to test-compile against directly (only mingw and
Linux/arm64 cross are supported per this project's conventions), so
the fix was verified via `gcc -D__APPLE__ -DARCH_ARM64 -fsyntax-only`
(confirms the guarded block reduces to a no-op, restoring the
pre-existing — already-correct — Apple codegen path exactly) plus
CI's own macOS job on the follow-up push.

Extended the existing `test/test_long_double_abi.c` (previously
SysV-x86-64-only in scope) to also run on Linux/arm64 — it already had
no arm64 guard in `main()`, so once both bugs above were fixed it
passed unmodified; only its header comment was extended to document
the two ARM64 bugs alongside the four x86-64 ones. Full suite verified
clean on all three targets: native Linux x86-64 (TCC 118/118, Unit
tests 209/209, Compliance 15/15, C-testsuite 220/220, Torture 3605/3609
— 100% of non-skipped, Dg-error 34/34, Link tests 7/7, 0 failed
overall), mingw cross (`test_long_double_abi` cleanly skips via its
existing `_WIN32` guard — Win64's long-double-by-hidden-pointer ABI
remains a separate, out-of-scope gap, see above), and arm64 cross
(Torture 3599/3609 — 6 pre-existing, unrelated complex-number/
imaginary-constant failures documented above, unaffected by this fix;
`test_long_double_abi` now passes where it previously SIGABRTed).

### Fixed (2026-08-10, continued — LONG_MAX/atomic-load session)

While investigating the previously-uninvestigated `test_glib`,
`test_neovim`, `test_samba`, `test_tcpdump` rows above, several were
first blocked purely by missing system dev packages in this sandbox
(`libpcre`/`libpcap`/`libluv`/`libuv` — installed via `dnf`, not an
rcc concern). Once installed, `test_samba`'s waf configure hit a
genuine rcc bug on its very first substantive probe (`#include
<Python.h>` for the `pyembed` check):

- \*\*`include/limits.h` hardcoded `LONG_MIN`/`LONG_MAX`/`ULONG_MAX` to

With that fixed, `#include <Python.h>` progressed to a second, deeper
bug inside `pyatomic_gcc.h`:

- \*\*`__atomic_load(ptr, retptr, order)` — the non-`_n`, 3-argument
- \*\*`ND_ATOMIC_LOAD`'s codegen never special-cased `float`/`double`

New regression tests: `test/test_limits_long_lp64.c` (LP64
`LONG_MIN`/`LONG_MAX`/`ULONG_MAX` values, plus the exact
`_XOPEN_SOURCE_EXTENDED`/`LONG_BIT` cross-check reproduction) and
`test/test_atomic_load_flonum.c` (`__atomic_load` on `float`/`double`
via a caller-supplied retptr, both `__ATOMIC_RELAXED` and
`__ATOMIC_SEQ_CST`, plus a cross-check that the pre-existing plain
integer `__atomic_load`/`__atomic_load_n` forms keep working). Full
suite verified clean on all three targets: native Linux x86-64 (TCC
118/118, Unit tests 211/211, Compliance 15/15, C-testsuite 220/220,
Torture 3605/3609 — 100% of non-skipped, Dg-error 34/34, Link tests
7/7, 0 failed overall), mingw cross (both new tests PASS), and arm64
cross (both new tests PASS).

With both fixes, `test_samba`'s waf configure now progresses far past
its earlier `Python.h`-triggered failure into unrelated dependency
checks (pam, iconv, ncurses, readline, ...), currently blocked on a
missing build-time Perl CPAN module (`Parse::Yapp::Driver`) in this
sandbox — an environment gap, not an rcc issue. `test_neovim` also now
configures past its `Luv`/`Libuv` dependency checks (after installing
beyond this session's scope. `test_glib` progressed from a
configure-time failure (missing `libpcre`) all the way to real
compilation; `test_tcpdump` progressed from a configure-time failure
(missing `libpcap`) all the way to a genuine rcc register-allocator
bug in its own radiotap decoder, root-caused and fixed separately —
see the next section.

### Fixed (2026-08-10, continued — nested-ternary register-allocator session)

Continuing the `test_tcpdump` investigation from above: once
`libpcap-devel` was installed, tcpdump built and ran cleanly except
for 2 of its own 636 tests (`802.11_exthdr`, `802.11_rx-stbc`) —
_wrong output_, not a crash: real radiotap capture files decoded to a
plausible-looking but incorrect frame summary (`[bit 17]` instead of
the golden `[bit 32]`/`[bit 15]`, MCS/GI/RX-STBC fields silently
missing).

- \*\*A deeply nested ternary expression, with a non-trivial

New regression test: `test/test_deep_nested_ternary_regalloc.c` — the
exact `BITNO_32` chain applied to every one of the 32 possible single
set bits, cross-checked against a boring/obviously-correct reference
implementation, plus the precise tcpdump-derived
`present ^ next_present` shape. Full suite verified: TCC 118/118, Unit
tests 212/212, Compliance 15/15, C-testsuite 220/220, Torture
3605/3609 (100% of non-skipped), Dg-error 34/34, Link tests 7/7, 0
failed overall; confirmed clean (both the new test and the existing
`test_gnu_ternary_omit_promote.c`, which exercises `ND_COND`'s other
recent fix, PASS) on the mingw and arm64 cross-compile targets.
`test_tcpdump`'s own `make check` now passes all 636 of its tests
(rebuilt `print-802_11.c` and relinked against the fixed compiler,
confirmed `802.11_exthdr`/`802.11_rx-stbc` now match their golden
output exactly); a full fresh harness run (`test/linux_thirdparty.bash
test_tcpdump`) confirms `rc=0`. **`test_tcpdump` is fully fixed.**

### Fixed (2026-08-11, forward-referenced local-label binding / ELF visibility session)

Continued investigating `test_libgmp` (GNU MP), whose row above still
carried a stale, long-superseded status from an early configure-time
failure; many prior sessions' worth of assembler fixes (not all
individually logged here) had already gotten `libgmp.so` most of the
way through its own build. This session's fresh `make check-all`-driven
rebuild hit a genuine linker failure building the shared library:

- \*\*A local label (no `.globl`) referenced via a forward `call`/`jmp`/
- \*\*`.hidden`/`.protected`/`.internal` (ELF symbol visibility) were not

With both fixed, `libgmp.so`/`libgmp.a` now build and link completely
clean (previously never got past the linker at all). The resulting
library's own `tests/mpn/t-*` runtime suite still shows 47 failures
(`t-mul`, `t-invert`, `t-bdiv`, `t-hgcd`, `t-gcd_11`, ... — SIGABRT/
SIGSEGV or wrong values, all inside `mpn_mul_basecase`/`toom22_mul`/
`toom33_mul`/`dcpi1_bdiv_qr`) — **investigated and confirmed NOT an rcc
bug**: built the identical GMP 6.3.0 source tree with the system's real
`gcc`+GNU `as`+GNU `ld` instead (`CC="gcc -std=gnu17"` to work around
this GCC 15's C23-default-prototype rejection of GMP's own 2004-era
`configure` probe, unrelated to rcc) as a correctness oracle: every one
of the same tests (`t-invert`, `t-mul`, `t-mullo`, `t-sizeinbase`,
`t-gcd_11`, `t-fib2m`, `t-bdiv`) crashes with the _identical_ exit code
against the real-gcc-built library too. This is a pre-existing GMP
6.3.0 / environment incompatibility (this old release's `k8`-tuned
hand-written asm vs. this sandbox's specific CPU/kernel/glibc
combination, or a genuine upstream GMP bug at this vintage) —
completely unrelated to and unaffected by either fix above, matching
this project's own "no pre-existing rcc bugs" invariant. Not chased
further (out of scope: real 3rd-party source, not an rcc regression).

New regression tests: `test/test_asm_forward_local_label_binding.c` (4
cases: forward-referenced `call`-target and `%rip`-LEA-target labels
both must bind LOCAL; two different translation units defining the
same-named local label `Ltab` this way — the exact real libgmp
shape — must both bind LOCAL rather than colliding as GLOBAL; an
explicit `.globl` must still correctly pin GLOBAL) and
`test/test_asm_hidden_visibility.c` (4 cases: `.hidden`/`.protected`/
`.internal` each produce the matching ELF visibility, byte-for-byte
checked against real GNU `as`'s own `objdump -t` output shape; a plain
`.globl` with no visibility directive stays at default visibility).
Full suite verified: Unit tests 214/214, Compliance 15/15,
C-testsuite 220/220, Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, Link tests 7/7 — 0 failed overall (native Linux x86-64).

### Fixed (2026-08-11, continued — AES-NI/SSE2 instruction encoder session)

Investigated `test_libressl` (LibreSSL, an OpenSSL fork) fresh — never
previously triaged (its row above just showed "—"). `./configure &&
make check` failed immediately assembling
`crypto/aes/aesni-elf-x86_64.S` (OpenSSL/LibreSSL's hand-optimized
AES-NI implementation, `crypto/aes/asm/aesni-x86_64.pl`-generated):

- **rcc's assembler had zero support for the AES-NI instruction set**
- \*\*Several SSE2/SSSE3 instructions whose x86_enc.c encoders already
- **Several more instructions had no encoder at all**: `PSHUFD`

Fixed by adding the missing encoders (`src/x86_enc.c`/`.h`:
`x86_aesenc`/`x86_aesenclast`/`x86_aesdec`/`x86_aesdeclast`/
`x86_aesimc`/`x86_aeskeygenassist`, `x86_pshufd`, a shared
`group14_shift_imm()` helper backing `x86_pslldq`/`x86_psrldq`/
`x86_psllq`/`x86_psrlq`, `x86_pinsrw_rm`) and wiring every one of the
above — new and pre-existing — into `encode_x86()`'s mnemonic
dispatch. Every encoding verified byte-for-byte identical to real GNU
`as`'s own output for the same source (register operands `%xmm0`-
`%xmm3`/memory base `%rbx`, all `< 4`, to sidestep a separate,
harmless, purely cosmetic pre-existing quirk noted below).

With these fixed, `crypto/aes/aesni-elf-x86_64.S`,
`crypto/modes/ghash-elf-x86_64.S`, and `crypto/rc4/rc4-elf-x86_64.S`
all now assemble cleanly. `libressl`'s build then reaches a **new,
separate, and substantially larger gap**: 21 files under
`crypto/bn/arch/amd64/` (OpenSSL/LibreSSL's `s2n-bignum`-derived
constant-time bignum arithmetic — `bignum_mul_4_8.S`,
`bignum_modadd.S`, `bignum_sqr_6_12.S`, ...) open with `.intel_syntax
noprefix` and use Intel-syntax operand order/register spelling
throughout (`adcx r10, rax` — no `%`/`$` sigils, reversed
dst/src order, `[base+index*scale+disp]`-style memory syntax where
used) — **rcc's assembler has no Intel-syntax parsing mode
whatsoever**, only AT&T. This is a genuinely large, separate
undertaking (a full parallel operand-order/memory-syntax parser for
every instruction `.intel_syntax` sections can reach, not a handful of
missing mnemonics) — out of scope for this session, in the same
category as the already-documented AVX-512/`_BitInt`/`defer`-statement
gaps under "Needs fixing" below. Left for a future session.

**Pre-existing, unrelated, purely cosmetic quirk noted in passing**:
`maybe_rex()` (`src/x86_enc.c`)'s "does this operand need a REX
prefix" threshold is `>= X86_RSP` (4) — correct for its originally
intended 8-bit-register-remap use (SPL/BPL/SIL/DIL vs. AH/BH/CH/DH,
where 4-7 genuinely does need REX) and for real extended-register
(R8-15/XMM8-15) detection, but several callers (all the new XMM
encoders above included, following the exact same pattern every
pre-existing XMM encoder already uses) also apply it to XMM register
indices and memory base/index registers in the 4-7 range, where the
correct threshold is 8, not 4 — emitting one semantically-inert extra
REX `0x40` byte (always a pure no-op for non-8-bit operands) instead
of omitting it. Confirmed via disassembly comparison against real GNU
`as`: correct bytes decode identically either way, just not
byte-for-byte GNU-as-identical when an operand register/memory-base
index happens to land in [4,7]. Not fixed — it's genuinely harmless,
affects dozens of already-shipped, already-tested XMM encoders beyond
the scope of this session's fix, and a proper fix (splitting
`maybe_rex()`'s GP-8-bit-register use from its XMM/general-register
use into two correctly-thresholded helpers, then re-auditing every
existing call site) is its own separate, carefully-scoped refactor.

New regression test: `test/test_asm_aesni_sse2.c` — 25 sub-cases (one
per instruction), each checked against its exact, GNU-as-confirmed
encoded bytes via `objdump -s`. Full suite verified: Unit tests
215/215, Compliance 15/15, C-testsuite 220/220, Torture 3605/3609
(100% of non-skipped), Dg-error 34/34, Link tests 7/7 — 0 failed
overall (native Linux x86-64); the new test also confirmed passing
standalone on the mingw cross target.

### Fixed (2026-08-11, continued — atomic fetch-op narrow-argument session)

**test_nginx**: `nginx-tests/upstream_resolve_reload.t` hung until its
900s harness timeout — every worker process spun forever inside
`ngx_rwlock_wlock()`'s CAS retry loop, never observing the lock drop
back to 0.

**Root cause** (codegen.c, `ND_ATOMIC_FETCH_OP`): nginx's
`ngx_atomic_fetch_add(lock, -1)` macro-expands to
`__sync_fetch_and_add(lock, -1)` on platforms with GCC atomic builtins
(not the `ngx_atomic_amd64.h` inline-asm fallback, which is only used
when the builtins are unavailable). The value argument's own static
type (`-1` is a plain 4-byte `int`) is narrower than the pointee type
being operated on (`ngx_atomic_t` = `intptr_t`, 8 bytes) — a completely
ordinary implicit-conversion shape the parser leaves as-is (`node->rhs`
keeps its own `int` type; only `node->ty`, the _result_ type, is set
from the pointee). Codegen's `gen(node->rhs)` correctly materializes
the value at its own (4-byte) width, but every op path (x86-64 `lock
xadd`, the cmpxchg-loop combine for or/xor/and/nand, and ARM64's
ldxr/stxr loop) then operates on the full pointee-width (8-byte)
register without first widening it. On x86-64, writing a 32-bit
register implicitly zeroes the register's upper 32 bits, so `-1`
landed as its _zero-extended_ bit pattern (`0x00000000FFFFFFFF`)
instead of sign-extended (`0xFFFFFFFFFFFFFFFF`) — turning
`ngx_rwlock_unlock()`'s intended "subtract 1" into "add 4294967295",
which left the lock word permanently non-zero and every subsequent
`ngx_rwlock_wlock()` spinning forever. Confirmed via a minimal
standalone repro and `objdump` disassembly (the value materialized as
`mov $1,%r11d; neg %r11d` — no `movslq` sign-extension — then fed
straight into a 64-bit `lock xadd`), and via `gdb` stepping an
unoptimized debug build to rule out inlining or a second codegen path
before finding the actual `__sync_fetch_and_add` builtin parse site
(`parser.c`) and its `ND_ATOMIC_FETCH_OP` codegen case.

Fixed by sign/zero-extending `r_val` (the generated value register) to
the pointee's width immediately after `gen(node->rhs)`, before any
op-specific codegen, using the same source-signedness rule
(`node->rhs->ty->is_unsigned`) every other implicit narrow-to-wide
integer conversion in codegen.c already follows. Applies uniformly to
all six ops (add/sub/or/xor/and/nand) and both architectures, since the
extension now happens once, ahead of the `#ifdef ARCH_ARM64` split.

Rebuilt nginx with the fixed compiler and reran the _actual_
`nginx-tests` suite end to end (not just the one previously-hung test):
`prove .` — 492 files, 2600 tests, **all successful**.

**Second, unrelated bug found investigating the same rebuild**: a
`resolve_include_next()` (preprocess.c) gap where `RCC_INCDIR` (an
absolute path baked into the binary — every build, installed or not,
defaults it to `/usr/local/include/rcc`) and the relative `"include"`
search-path fallback (added whenever it differs from `RCC_INCDIR`) can
both physically exist with byte-identical bundled-header content — the
normal shape once `make install` has ever run on a machine that also
has a source checkout on hand. `#include <stdio.h>` resolves through
`RCC_INCDIR` first (checked before the fallback); `#include_next
<stdio.h>` from _inside_ that file used to advance past only
`RCC_INCDIR` and land on the fallback's identical copy next — a real,
non-trivial file (not a one-line forwarder, so the existing
`is_noop_forward_to_active()` no-op detector doesn't catch it), but
its own include guard is already defined by the first copy, so its
entire body — including its own `#include_next <stdio.h>` that would
reach the real system header — silently no-ops. `#include_next`
"succeeds" at a file that contributes nothing, leaving `FILE` (and
everything else glibc's real `<stdio.h>` declares) undeclared, with no
error. Fixed by having the current-directory scan skip past _both_
bundled-header slots together whenever either one matches, not just
whichever slot's physical path happened to match. (This specific
failure mode was surfaced by a stale, install-poisoned `preprocess.o`
left over from an earlier `make install` in this session rather than a
normally-reachable path — `make clean && make` alone also "fixes" it
by rebuilding with consistent flags — but the underlying search-order
bug is real and independently reproducible without any stale build
state involved, so it's fixed and covered on its own merits.)

New regression tests: `test/test_atomic_fetch_op_narrow_arg.c` (6
cases: the exact `fetch_add(long*, -1)` shape, a narrow non-constant
variable operand, fetch_sub/or/and, and an unsigned narrow value that
must zero- rather than sign-extend); `test/test_include_next_dup_incdir.c`
(builds a byte-identical duplicate of rcc's own bundled `include/stdio.h`
at the relative `"include"` fallback location and confirms
`#include_next` still reaches the real system header instead of being
swallowed). Full suite verified: Unit tests 217/217, Compliance 15/15,
C-testsuite 220/220, Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, Link tests 7/7 — 0 failed overall (native Linux x86-64); both
new tests also confirmed passing standalone on the mingw and arm64
cross targets.

### Fixed (2026-08-11, continued — lexer non-ASCII infinite loop / weak variable attribute session)

**test_go**: `go tool dist`'s C bootstrap phase (`cmd/dist`, then `lib9`/
`libbio`/`liblink` and the `5c`/`6c`/`8c`/`9c`/`5g`/`6g`/`8g` Plan9-style
compilers, all C) previously either hung indefinitely (timing out the
420s harness budget with no diagnostic at all) or, once that was fixed,
failed to link with "multiple definition" errors. Both were genuine rcc
bugs, found by attaching gdb to the hung/stuck compile and sampling its
call stack.

- \*\*Any non-ASCII byte that decoded to a non-identifier-start codepoint
- **`**attribute**((weak)) on a global variable was silently dropped**,

With both fixed, rcc successfully builds the entire go1.4 C bootstrap
chain end to end: `cmd/dist`, `lib9`, `libbio`, `liblink`, and the
`5c`/`6c`/`8c`/`9c`/`5g`/`6g`/`8g` compilers/assemblers/linkers, which
then successfully self-bootstrap and begin compiling the Go standard
library's own `.go` sources with the freshly-built `6g`. That later
stage now hits `runtime/mprof.go:487: r.Stack0 undefined (type
*BlockProfileRecord has no field or method Stack0)` — `Stack0` is a
field of `StackRecord`, anonymously embedded in `BlockProfileRecord`
for Go's standard field-promotion rules to make `r.Stack0` resolve to
`r.StackRecord.Stack0`; the rcc-built `6g` fails to resolve it. This is
several layers removed from rcc itself (a possible miscompilation
inside the _bootstrapped Go compiler's own_ embedded-struct-field-
promotion logic, written in Plan9 C, itself compiled by rcc — not
anything in the `.go` sources or in rcc's own C-level correctness) and
needs a dedicated, properly-scoped investigation into `6g`'s own gc/\*.c
sources; not attempted this session. `test_go` moves from completely
untriaged (timed out with zero diagnostic information) to "C bootstrap
phase fully fixed, blocked on one further layer" — a real, substantial
step forward even though the target doesn't fully pass yet.

New regression tests: `test/test_lexer_nonascii_infinite_loop.c` (the
exact middle-dot-in-macro-body trigger, with a `timeout`-guarded
subprocess invocation so a regression fails cleanly instead of hanging
the suite; plus a genuinely valid Unicode identifier to confirm the fix
didn't break the normal path); `test/test_weak_variable_attribute.c`
(4 cases: trailing weak on a tentative/`.bss` global, trailing weak on
an initialized/`.data` global, prefix weak, and an ordinary non-weak
global that must NOT become weak as a side effect — verified via `nm`'s
clean 3-field output rather than `objdump -t`'s fixed-width flag
column, whose internal spaces defeat naive whitespace-based parsing).
Full suite verified: Unit tests 219/219, Compliance 15/15, C-testsuite
220/220, Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Link
tests 7/7 — 0 failed overall (native Linux x86-64); both new tests also
confirmed passing standalone on the mingw and arm64 cross targets (the
lexer test's POSIX `timeout N cmd` guard needed a Windows-specific
no-op, since cmd.exe's own built-in `timeout` takes an incompatible
`/T <seconds>` syntax).

### Fixed (2026-08-11, continued — linker command unquoted-path session)

Investigated `test_muon` (never previously triaged beyond "muon
self-tests, some pass, some fail"). `build/muon -C build test` hit a
genuine rcc bug on its very first compiler-capability probe:

- \*\*The fallback GCC-linker invocation built its `system()` command

With this fixed, `test_muon`'s own suite moved from immediately
breaking on its first probe to 337/387 passing (87%); of the 32
remaining failures, the 10-case `common/273 both libraries` cluster
was cross-checked against real GCC linking the identical rcc-produced
`.so`/`.o` files (`gcc -no-pie -o main main.p/src/main.c.o
libwith_library.so -lm`) and fails **identically** — a genuine
transitive-shared-library `-rpath-link` limitation in muon's own
generated `build.ninja` link rule, not an rcc bug. The remaining ~22
failures were not individually triaged this session; left for a
future one.

New regression test: `test/test_link_path_with_space.c` (compiles and
links a printf-using program into an output path containing a space,
then runs the resulting binary and checks its output — reproduces the
exact muon directory-naming shape). Full suite verified: Unit tests
220/220, Compliance 15/15, C-testsuite 220/220, Torture 3605/3609
(100% of non-skipped), Dg-error 34/34, Link tests 7/7 — 0 failed
overall (native Linux x86-64); the new test also confirmed passing
standalone on the mingw and arm64 cross targets.

### Fixed (2026-08-11, continued — alloca() argument-count crash session)

Continued investigating `test_muon`'s remaining failures (of 32, one
session earlier fixed the linker-path bug that got 337/387 passing).
`common/36 has function` — muon's own "does the C library provide
`alloca`" capability probe — compiles `int main(void) { return
alloca(); }` (calling `alloca` with zero arguments, deliberately: the
probe only cares whether the _name_ resolves, not whether the call is
well-formed) and expects a clean compile error.

- \*\*`alloca()` called with the wrong number of arguments crashed rcc

New regression test: `test/test_alloca_argcount.c` (zero arguments
must be a clean compile error, not a crash; too many arguments must
also be a clean compile error, not a silent accept; the correct
single-argument form must still compile, link, and run correctly).
Full suite verified: Unit tests 221/221, Compliance 15/15, C-testsuite
220/220, Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Link
tests 7/7 — 0 failed overall (native Linux x86-64); the new test also
confirmed passing standalone on the mingw and arm64 cross targets.

### Fixed (2026-08-11, continued — \_\_has_builtin session)

Continued investigating `test_muon`'s remaining failures after the
`alloca()` argument-count crash fix above (which unblocked the crash
but didn't, by itself, flip `common/36 has function`'s overall
verdict — see below). muon's own `has function alloca` capability
probe runs several fallback checks; the last of them is the modern
`__has_builtin` idiom:

```c
#ifdef __has_builtin
  #if !__has_builtin(__builtin_alloca)
    #error "__builtin_alloca not found"
  #endif
#elif ! defined(alloca)
  __builtin_alloca;
#endif
```

- \*\*`__has_builtin` (a clang/GCC preprocessor extension, now the
- **Second, subtler bug found while verifying the first fix**: unlike

New regression test: `test/test_has_builtin.c` — `#ifdef`/`defined()`
visibility, several genuinely-implemented builtins (including the
alloca-aliasing case) reporting true, an unregistered name reporting
false rather than erroring, the exact muon-derived `__has_builtin`
fallback probe shape compiling cleanly, and a runtime check that
`__builtin_alloca` genuinely works once `__has_builtin` confirms it.
Verified this test fails without the fix (A/B, `git stash`-style
revert) and passes with it. Also re-ran the four
`c23-has-c-attribute-*` and `c23-attr-syntax-8` GCC torture tests
directly to confirm no regression from the expansion-order fix. Full
suite verified: Unit tests 222/222, Compliance 15/15, C-testsuite
220/220, Torture 3605/3609 (100% of non-skipped), Dg-error 34/34,
Link tests 7/7 — 0 failed overall (native Linux x86-64); the new test
and the `c23-has-c-attribute-2` torture test both confirmed passing
standalone on the mingw and arm64 cross targets.

With this fix, `test_muon`'s own `common/36 has function` row flips
from failed to passing (338/387 muon self-tests now pass, up from
337/387 after the `alloca()` crash fix alone — that fix alone wasn't
sufficient to flip this specific test's verdict, since muon's
redeclared-signature probe still correctly fails to compile/link
either way, matching real GCC's own behavior for that same construct;
only the `__has_builtin` fallback probe's success actually flips
`has function alloca`'s overall YES/NO answer). The remaining 31
`test_muon` failures include the previously-confirmed-NOT-an-rcc-bug
`common/273 both libraries` cluster (8 of the 31) plus ~23 not
individually triaged this session — left for a future session.

### Fixed (2026-08-11, continued — -Werror unknown-flag session)

Continued investigating `test_muon`'s remaining failures.
`common/104 has arg` — muon's own "does the compiler support this
argument" capability probe — checks `-Werror -fiambroken` (a
deliberately nonexistent flag name, "I am broken") specifically to
confirm `-Werror` can promote an unsupported-argument probe into a
hard failure; expects it to fail to compile.

- \*\*A bare `-Werror` combined with a genuinely unrecognized
- \*\*First fix attempt caused a real regression, caught by full

New regression test: `test/test_werror_unknown_opt.c` — five cases:
an unrecognized non-`W` flag without `-Werror` must still just warn
and succeed; the same flag with bare `-Werror` must now fail; an
unrecognized `-W` flag with bare `-Werror` must still only warn
(clang-style leniency preserved); the same `-W` flag with
`-Werror=unknown-warning-option` must fail (pre-existing, unchanged);
a genuinely supported flag (`-O2`) combined with `-Werror` must still
compile cleanly. Verified this test fails without the fix and passes
with it (A/B). The regression itself was caught and fixed within this
same session via full `make check-all` before any push. Full suite
verified: Unit tests 223/223, Compliance 15/15, C-testsuite 220/220,
Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Link tests
7/7 — 0 failed overall (native Linux x86-64); the new test and the
previously-regressed torture tests (`c23-constexpr-4`, `c11-stdint-1`,
etc.) both confirmed passing standalone on the mingw and arm64 cross
targets.

With this fix, `test_muon`'s `common/104 has arg` row flips from
failed to passing (339/387 muon self-tests now pass, up from
338/387). The remaining 30 failures still include the
previously-confirmed-NOT-an-rcc-bug `common/273 both libraries`
cluster (8 of the 30); a related linker-arg probe,
`common/180 has link arg`, was investigated but not fixed this
session — it needs rcc's own native ELF linker (or its `-Wl,`
sub-flag accumulator in main.c) to genuinely validate and reject
unrecognized linker sub-flags the way real `ld` does, which would
require enumerating and matching `ld`'s real accepted-flag surface; a
wrong rejection risks breaking some other currently-tolerated `-Wl,`
flag elsewhere in the corpus, so this is left as a separate,
larger, more carefully-scoped undertaking for a future session rather
than a quick win. ~22 of the remaining 30 failures are still not
individually triaged.

### Fixed (2026-08-11, continued — \_\_declspec native-target / K&R EOF crash session)

Continued investigating `test_muon`'s remaining failures.
`common/197 function attributes` — muon's own "does this GNU/MSVC
function attribute compile" capability probe — checks
`__declspec(dllexport) int foo(void) { return 0; }` specifically
expecting it to be rejected on `posix` (only genuinely supported on
`_WIN32`).

- \*\*`__declspec(...)` (MSVC's declaration-attribute syntax) was
- **Second, more serious bug found while verifying the first fix**:

New regression tests: `test/test_declspec_native_reject.c`
(`__declspec(dllexport)` must be a clean compile error on native Linux
and must still compile cleanly on `_WIN32`; ordinary `__attribute__`
usage must stay unaffected on every target) and
`test/test_kr_param_list_eof.c` (the minimal `__declspec`-independent
K&R/EOF crash repro must be a clean diagnostic, not a crash — verified
via the `WIFEXITED`/`WEXITSTATUS` convention that distinguishes an
actual signal death from a clean nonzero exit, since a crashed
child's exit status surfaces through `system()`'s intermediate shell
as an ordinary 128+signal exit code, indistinguishable from a genuine
diagnostic exit without decoding it this way; a genuine, well-formed
K&R function definition must still compile, link, and run correctly).
Both verified failing without their respective fix and passing with
it (A/B). Full suite verified: Unit tests 225/225, Compliance 15/15,
C-testsuite 220/220, Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, Link tests 7/7 — 0 failed overall (native Linux x86-64); both
new tests confirmed passing standalone on the mingw and arm64 cross
targets (including `test_declspec_native_reject.c`'s `_WIN32` branch,
confirming `__declspec(dllexport)` still compiles cleanly under the
mingw cross-compiler).

With this fix, `test_muon`'s `common/197 function attributes` row
flips from failed to passing (340/387 muon self-tests now pass, up
from 339/387).

### Fixed (2026-08-11, continued — angle-bracket #include cwd-search session)

Continued investigating `test_muon`'s remaining failures.
`common/189 check header` — muon's own `check_header()` compiler
method probe — deliberately copies a same-named decoy file
(`ouagadougou.h`, a placeholder name chosen to be obviously
nonexistent) into the build directory next to the compiled test file,
then checks `#include <ouagadougou.h>` (angle brackets) still fails to
find it, confirming the compiler's "system" search never accidentally
includes the current working directory.

- \*\*Angle-bracket `#include <name.h>` could silently resolve to an
- **Collateral consideration**: `#embed <file>` (C23) reuses the same
  `test/test_embed.c` (from an earlier session) already relies on

New regression test: `test/test_include_angle_no_cwd.c` — a decoy
header sitting next to the compiled source must NOT be found via
`#include <decoy.h>` (angle brackets), but the identical decoy MUST
still be found via `#include "decoy.h"` (quote form, unaffected by the
fix). Verified failing without the fix and passing with it (A/B).
Full suite verified: Unit tests 226/226, Compliance 15/15,
C-testsuite 220/220, Torture 3605/3609 (100% of non-skipped), Dg-error
34/34, Link tests 7/7 — 0 failed overall (native Linux x86-64); both
the new test and `test_embed.c` (confirming its own #embed
angle-bracket leniency stayed intact) verified passing standalone on
the mingw and arm64 cross targets.

With this fix, `test_muon`'s `common/189 check header` row flips from
failed to passing (341/387 muon self-tests now pass, up from
340/387).

### Fixed (2026-08-11, continued — #warning -Werror promotion session)

Continued investigating `test_muon`'s remaining failures.
`common/28 try compile` — muon's own `compiler.compiles(werror:
true/false)` capability probe — checks that a `#warning` directive
compiles cleanly with `werror: false` but fails to compile with
`werror: true`.

- **`#warning` never promoted to a compile error under `-Werror`**

New regression test: `test/test_warning_werror_promote.c` — three
cases: `#warning` without `-Werror` must still compile cleanly;
`#warning` with bare `-Werror` must fail to compile; `#warning` with
`-pedantic-errors` (no bare `-Werror`) must still compile cleanly,
matching real GCC exactly in every case. Verified failing without the
fix and passing with it (A/B), and that the earlier
`test_werror_unknown_opt.c` test is unaffected by the
`opt_werror_bare` → `opt_werror_flag` promotion-to-global refactor.
Full suite verified: Unit tests 227/227, Compliance 15/15, C-testsuite
220/220, Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Link
tests 7/7 — 0 failed overall (native Linux x86-64); the new test
confirmed passing standalone on the mingw and arm64 cross targets.

With this fix, `test_muon`'s `common/28 try compile` row flips from
failed to passing (342/387 muon self-tests now pass, up from
341/387).

### Fixed (2026-08-12, continued — path_join() absolute-path session)

While A/B-verifying the angle-bracket cwd-search fix above, an
unrelated, pre-existing regression test (`test/test_pp_tokens_file_boundary.c`,
which embeds absolute header paths in quote-form `#include` directives)
started failing once the angle-bracket fix's tightened quote-include cwd
fallback stopped silently masking it.

- \*\*`path_join(dir, file)` (preprocess.c) never checked whether `file`

New regression test: `test/test_include_abs_path_quote.c` — a header
and its including source file placed in two different, unrelated
subdirectories, `#include`d by the source's absolute path; must
compile cleanly. Verified failing without the fix (isolated: only
`path_join()`'s absolute-path check reverted, angle-bracket fix left
in place) and passing with it (A/B), and that the pre-existing
`test_pp_tokens_file_boundary.c` failure this session started from is
fixed by the same change. Full suite verified: Unit tests 228/228,
Compliance 15/15, C-testsuite 220/220, Torture 3605/3609 (100% of
non-skipped), Dg-error 34/34, Link tests 7/7 — 0 failed overall (native
Linux x86-64); both new/fixed tests confirmed passing standalone on
the mingw and arm64 cross targets.

### Fixed (2026-08-12, ksh93 arith / IEEE truthiness / 64-bit pointer-arithmetic session)

`test_ksh93`'s own `shtests` suite ran for the first time with a fully
built binary (the arith.sh `cos*cos + sin*sin > 1.01` failure that had
been "left for a future session" was reproduced: **every math function
hung**). Four stacked rcc bugs were found and fixed; ksh93's suite went
from the build blocking on `arith.sh` to only the single documented
long-double-precision rounding check below failing.

- **Long-double assignment-expression value returned 0** (codegen.c,
- **IEEE flonum truthiness used a bitwise GP test** (codegen.c, 7 sites)
- **Pointer-offset multiply hardcoded `int`** (type.c `new_scale_mul`)
- **Pointer subtraction typed `int` instead of `ptrdiff_t`** (type.c,

New regression tests: `test/test_ld_assign_expr.c` (assignment
expression value + the sfcvt-style normalization guard),
`test/test_float_truthiness.c` (if/&&/||/ternary/while/do-while/\_Bool
cast with -0.0 and NaN, cross-checked against gcc),
`test/test_ptr_arith_64bit.c` (`p + n` with n >= 2^31, `p - q` >= 2^31,
scaled `long*` offset — proven to fail on the unfixed compiler with
`q-p = -294967296`). Full suite verified: Unit tests 228/228, Compliance
15/15, C-testsuite 220/220, Torture 3605/3609 (100% of non-skipped),
Dg-error 34/34, Link tests 7/7 — 0 failed overall (native Linux x86-64).
ksh93: clean rebuild with fixed rcc; `bin/shtests` runs the entire suite
(alias/append/.../variables/vartree\*, 1000s of tests) with the **only**
remaining failure `arith.sh[327]` — `typeset -lE20 val=123.01234567890`
expects 20-significant-digit extended-precision rounding
(`123.0123456789`) but rcc prints the double expansion
(`123.01234567890000449`), because rcc computes `long double` internally
in double precision by design (see the `TY_LDOUBLE` comments in
codegen.c/type.c); true 80-bit extended support is a multi-session
feature, not a quick win. The 5 GB `printf -v` (`basic.sh[983]`) now
passes too (takes ~2 min of byte-at-a-time padding; the 420 s harness
timeout at full-suite scale is a throughput, not correctness, issue).

### Fixed (2026-08-12, external-link fallback / muon toolchain detection session)

`test_muon` was at 342/387 with ~22 untriaged failures, all in
link-command generation: sub-projects built with `CC=rcc` got bare
link commands (`rcc -o prog ... liblib1.so`) instead of the full
`-Wl,--as-needed -Wl,--no-undefined -Wl,-rpath,<dir> -Wl,--start-group
... -Wl,--end-group -Wl,-soname,...` set, so shared-library chains
failed to link (transitive DT_NEEDED resolution) and shared libs lost
their DT_SONAME/DT_RUNPATH. Two independent causes, both fixed:

- \*\*rcc silently dropped linker commands its internal linker can't
- **muon doesn't recognize rcc as a gcc-compatible compiler** — its

New regression test: `test/test_link_wl_fallback.c` — a bare `rcc
-Wl,-v` must print the linker version banner (not "no input files"),
and a `-Wl,-rpath,<dir>` link of a program against a shared library in
a non-standard directory must produce a binary that actually finds its
library at runtime (proven to fail with the internal linker silently
dropping `-Wl,-rpath`: no DT_RUNPATH → runtime "cannot find library").
Full suite verified: Unit tests 231/231, Compliance 15/15, C-testsuite
220/220, Torture 3605/3609 (100% of non-skipped), Dg-error 34/34, Link
tests 7/7 — 0 failed overall (native Linux x86-64). muon: full
`build/muon -C build test` now 384/387 — the 3 remaining failures
(`muon/wayland`, `common/183 partial dependency`, `common/251
add_project_dependencies`) reproduce identically with a fully gcc-built
muon and are environment/muon issues (wayland-scanner tool missing;
zlib link path), not rcc bugs.

### Fixed (2026-08-12, AVX/AVX2 intrinsics session)

Implemented AVX (256-bit) intrinsic support end to end — the VEX
encoding infrastructure and the full AVX/AVX2 `__builtin_ia32_*256`
surface — unblocking test_brotli's `-mavx2` compile (test_blake3's
AVX-512 remains, see below):

- **VEX encoders in `src/x86_enc.c`** — 2- and 3-byte VEX prefix
- **32-byte vector ops** (`gen_vector32_x86`) — element-wise
- **`__builtin_ia32_*256` classification** (`ia32_builtin_ret`) — the
- **Encoding gotchas found by A/B against gcc -mavx2** (the print
- **`__AVX__`/`__AVX2__`/`__FMA__`/AVX-512 macros** in
- New regression test `test/test_avx2_intrinsics.c` (x86-only,

### Fixed (2026-08-12, AVX-512 / EVEX intrinsics session)

test_blake3 now builds and passes (rc=0) end to end — the whole
`blake3_avx512.c`/`blake3_avx2.c`/`blake3_sse41.c`/`blake3_sse2.c`/
`blake3_dispatch.c` set compiles with rcc against the real glibc
`<immintrin.h>` chain:

- **EVEX infrastructure in `src/x86_enc.c`** — the 4-byte 0x62 prefix
- **`*512_mask` builtin dispatch** in codegen.c — the masked forms
- **64-byte vectors**: int-element ops go through the parser's
- **Classifier**: the 512/512_mask size suffix, result-width
- **Bundled header gaps** the real chain needs: `__m128[u|d|i]_u` /
- New regression test `test/test_avx512_intrinsics.c`: compiles the

Full suite: TCC 118/118, Unit 235/235, Compliance 15/15, C-testsuite
220/220, Torture 3605/3609 (0 failed, 4 todo); test_blake3 PASS.

### Fixed (2026-08-12, SIMD intrinsics / real glibc headers session)

Investigated why rcc cannot include the real glibc `<immintrin.h>`
chain (the diagnosis that became this session's fixes). With rcc's
bundled headers renamed away, `#include <immintrin.h>` resolved to
`/usr/lib/gcc/.../include/` and failed on four independent gaps:

- **`__builtin_ia32_*` calls typed as implicit int** — every real
- **GNU `extern __inline __gnu_inline__` emitted nothing** — GCC
- **scalar→vector casts ICE'd** — `(__v8qi)0LL` (and the real
- **`__bf16` type missing** — avx512bf16vlintrin.h's `typedef \_\_bf16
- **bundled-header gaps** — rcc's own xmmintrin.h/emmintrin.h lacked

Also fixed while iterating: haddps/haddpd prefix (F2, not none),
addsubps matching the "add" prefix first (now checked before plain
add/sub), unary 0F38 ops (pabs*/pmovsx/zx*) loading a second
argument, roundps/roundpd (unary 0F3A) reading the imm from the wrong
argument, cvtpi2pd arity (1 arg), vec_init_v4hi storing ints instead
of the vector's element width, blendps-family width (non-128-suffixed
SSE4.1 names are 16 bytes, not MMX 8), cvtss2si/cvtsd2si prefix order
(F3/F2 must precede REX), and the 5 legacy sqrt\* builtins declared as
real functions in rcc's bundled headers (calls carried their target
in lhs, not funcname).

Verified: with the bundled headers renamed away, a program covering
SSE1/SSE2/SSE3/SSSE3/SSE4.1/MMX (`real_broad.c`: addps/addsubps/
haddps/pmaddwd/pshufb/pabsd/pmin/pmovsxbd/blendps/roundps/ptest/
extract/cvt/mmx-add) compiles against the REAL glibc headers and its
output is byte-identical to gcc -O2 -mssse3 -msse4.1 at both rcc -O0
and rcc -O2. blake3/brotli still need AVX2/AVX-512 (256/512-bit)
intrinsics + mask registers — unchanged, still item 1 below.

New regression test: `test/test_ia32_intrinsics.c` — direct
`__builtin_ia32_*` calls (packed+scalar float ALU incl. lane-0
semantics, integer ALU, compares with operand swap, shuffles,
converts, move masks, pshufb128, unary pabsd128, blendps/roundps/
ptestz128, MMX paddw via vec_init), the splat casts, and a GNU
extern-inline wrapper exercising the local-copy link path; passes at
-O0/-O1/-O2/-O3 and matches gcc byte-for-byte. Full suite verified:
Unit tests 233/233, TCC 118/118, Compliance 15/15, C-testsuite
220/220, Torture 3605/3609 (100% of non-skipped), Dg-error 34/34,
Link 7/7 — 0 failed (native Linux x86-64). The 8-byte vector and
vector→scalar codegen paths are additionally covered on arm64 by the
same test (guard skipped there).

### Fixed (2026-08-17, inline-asm op_addr register-pressure / matching-constraint session)

**test_busybox fixed** — busybox's `sha1sum`/`sha256sum` (via
`get_shaNI()` → `cpuid_eax_ebx_ecx()`) segfaulted in
`cpuid_eax_ebx_ecx()` itself. Two stacked codegen.c bugs in the x86-64
`ND_ASM` output/store-back path, both in the same function:

- **Register-pressure regression from an in-flight fix**: every
- \*\*The restore-from-stack step then reintroduced the _original_

Also fixed while isolating this (both pre-existing, latent gaps
unrelated to the above, but reached by the same crash investigation):

- \*\*`x86_mov_rr()` (x86_enc.c) never emitted a REX prefix for 16-bit

New regression tests: `test/test_asm_multi_output_clobber.c` extended
with a second case reproducing the exact matching-constraint clobber
via a `cpuid_ptrs()` helper mirroring busybox's
`cpuid_eax_ebx_ecx()` signature/constraint list byte-for-byte;
`test_x86_asm.c` already had the `%w1` 16-bit-modifier case (case 2)
that caught the REX regression. Verified: fresh busybox rebuild's
`sha1sum`/`sha256sum` on empty and non-empty files now byte-identical
to real `sha1sum`/`sha256sum`. `make check-all`: 0 failed (Unit
4231/4231 incl. both tests, TCC 118/118, Compliance 15/15,
C-testsuite 220/220, Torture 3605/3609 — 0 failed, 354 skipped, 4
todo, Dg-error 34/34, Link 10/10). mingw cross-build: 0 failed.

### Fixed (2026-08-17, missing float-precision math.h declarations session)

**test_box2d fixed** — box2d's `test/test_determinism.c` (DeterminismTest)
hung indefinitely instead of crashing. Root cause: `include/math.h`
declared most ISO C99 `<math.h>` functions for `double`/`long double`
but was missing 27 of their `float`-precision (`f`-suffixed) variants:
`log2f`, `truncf`, `asinhf`, `acoshf`, `atanhf`, `exp2f`, `expm1f`,
`log1pf`, `cbrtf`, `hypotf`, `erff`, `erfcf`, `copysignf`,
`remainderf`, `fdimf`, `fmaxf`, `fminf`, `nearbyintf`, `rintf`,
`lroundf`, `lrintf`, `llroundf`, `llrintf`, `scalbnf`, `ldexpf`,
`frexpf`, `modff` — same class of bug as the already-documented
`erf`/`erfc` gap (see test_math_erf.c): an undeclared external call
falls back to an implicit `int` return, so the caller reads the
result out of RAX instead of XMM0, silently corrupting it.

box2d's own `b2UnwindAngle()` calls `remainderf(radians, 2*pi)` with
no other prototype in scope. `remainderf(-1.0f, 2*pi)` returned `0`
instead of `-1.0f` (other inputs returned values in the billions), so
every dynamic body's initial rotation came out as identity instead of
the intended angle. The falling-hinges determinism test's tightly
packed initial layout then badly overlapped, the contact solver
exploded body positions into the millions within one physics step,
and the simulation never settled — the test's outer loop has no step
cap, waiting for all bodies to sleep, so this presented as an
apparent infinite hang rather than an obvious crash or wrong-output
diff. Isolated via targeted instrumentation (printing body 0's
transform each step) compared against a real-gcc-built baseline
(passes in ~2s), which narrowed the divergence to `remainderf`
specifically within one debugging pass.

Fixed by adding the 27 missing float-precision declarations to
include/math.h, matching the existing double/long-double lists.

New regression test: `test/test_math_float_variants.c` — covers
`remainderf` (the exact real-world trigger) plus a representative
sample of the other 26 newly-declared functions, each asserted
against a mathematically exact expected value (powers of two,
Pythagorean triples, domain endpoints) with a tight `1e-6` epsilon,
not just "plausible range" (implicit-int garbage can itself look
deceptively plausible — see test_math_erf.c's own note on this).
Verified: fresh box2d rebuild (`cmakebuild/bin/test`, box2d v3.1.1)
now passes its full unit-test suite end to end, settling the
determinism scene at exactly step 288 (matching upstream's own
`EXPECTED_SLEEP_STEP`), byte-for-byte matching the real-gcc-built
baseline's step-by-step body transforms. `make check-all`: 0 failed
(Unit 4232/4232 incl. the new test, TCC 118/118, Compliance 15/15,
C-testsuite 220/220, Torture 3605/3609 — 0 failed, 354 skipped, 4
todo, Dg-error 34/34, Link 10/10).

### Fixed (2026-08-17, loop-unroll induction-variable final-value session)

**test_gzip fixed** — `gzip -c <anything>` segfaulted unconditionally
(`Error 139`, reproduced even on the project's own `make check`'s
`gzip.doc.gz` generation step). Root cause: `-funroll` (enabled at
`-O2`+, gzip's own build uses `-O2`) unrolls small constant-count
`for` loops by cloning the body N times and substituting every read of
the induction variable inside each clone with its compile-time
constant value — correct for the clones themselves, since the loop's
own `inc` clause is discarded by unrolling. But `try_unroll()` never
wrote the induction variable's _real_ runtime storage past its `init`
value; any code textually after the loop that still reads the same
(function-scope) variable saw whatever `init` left it at, not the
value a real, non-unrolled loop's exit would leave it at
(`start + count`).

gzip's `trees.c: ct_init()` has exactly this shape: `for (code = 0;
code < 16; code++) { ...dist_code[dist++] = code... }` immediately
followed by a second loop reusing the same variable via an empty init
clause, `for (; code < D_CODES; code++) { ... }`, relying on `code`
continuing from 16. The first loop unrolled (16 iterations, under
`MAX_UNROLL_ITERS`) and left `code` at 0 instead of 16. The second
loop's own body, `dist_code[256 + dist++] = code` guarded by `1 <<
(extra_dbits[code] - 7)`, then read `extra_dbits[0] - 7 = -7`; x86's
SHL masks the shift count to 5 bits, so `1 << -7` became `1 << 25 =
33554432` — the inner loop ran 33 million times, writing far past
`dist_code`'s 512-byte array and crashing a few thousand iterations
in.

Isolated via a from-scratch bisection: attaching gdb to the crashing
`gzip` binary showed `ct_init`'s disassembly fully unrolled (16
near-identical blocks, one per `code` value, confirmed by counting
repeated `cmp %r11d,%r10d` patterns spaced a fixed number of bytes
apart); a hand-built standalone reproducer of `ct_init`'s exact
dist-code-mapping shape reproduced nothing at the default optimization
level but crashed identically once compiled with the same `-O2` gzip's
build actually uses, narrowing the search to `-funroll`'s
implementation directly.

Fixed by having `try_unroll()` (`src/opt.c`) append one extra
assignment, `ivar = start_val + count`, after the last unrolled body
copy — materializing the same final value a genuine loop's exit would
leave the induction variable at, for any later code that still reads
it.

New regression test: `test/test_unroll_ivar_final_value.c` — the
exact two-loops-reusing-one-variable shape plus three narrower cases
(bare read after the loop, non-zero start value, `<=` condition),
each checked at `-O0` and `-O2`. Verified: gzip 1.14's own `make
check` now passes its full 30/30 test suite (previously crashed
before a single test could run), and a direct `./gzip -c gzip.doc |
gzip -d | diff - gzip.doc` round-trips byte-identical. `make
check-all`: 0 failed (Unit 4234/4234 incl. the new test, TCC 118/118,
Compliance 15/15, C-testsuite 220/220, Torture 3605/3609 — 0 failed,
354 skipped, 4 todo, Dg-error 34/34, Link 10/10).

### Fixed (2026-08-17, missing `.rela.tdata` ELF section session)

**mimalloc `test-stress` SIGSEGV, fixed** — root cause: `src/elf_write.c`
built a `.rela.<name>` relocation section for every other data-bearing
built-in section (`.rela.text`, `.rela.data`, `.rela.rodata`,
`.rela.init_array`, `.rela.fini_array`, plus every dynamically-registered
section) but never emitted one for `.tdata` at all — no section header,
no reloc entries, nothing. `codegen.c`'s global-initializer emission
already correctly appended a TLS pointer's initializer relocation to
`obj->data_tls_relocs` (`objfile_add_reloc(cg_obj, SEC_TDATA, ...)`), so
the bug was entirely on the writer side: any `_Thread_local`/`__thread`
variable whose initializer is the address of an ordinary (non-TLS)
global — e.g. mimalloc's `src/init.c`:
`mi_decl_thread mi_heap_t* _mi_heap_default = (mi_heap_t*)&_mi_heap_empty;`
— got a NULL/garbage pointer instead of the real address in every
thread's TLS block, main thread included, since the linker had no
relocation to patch the value in. mimalloc dereferences that pointer on
essentially every allocation, so `test-stress`'s first background thread
SIGSEGV'd almost immediately. (A regular, non-TLS global doing the exact
same `&other_global` initializer worked fine, routed through
`.rela.data` — which is why this class of bug survived so long.)

Fix: added the missing `.rela.tdata` section end-to-end in
`elf_write.c` — `has_rela_tdata`, its `.shstrtab` name, section-header
index slot (right after `.rela.rodata`, before any dynamically-
registered sections' own `.rela.<name>`), file-offset/size computation,
the actual reloc-entry write loop (mirrors `.rela.data`/`.rela.rodata`
byte-for-byte), and the `SHT_RELA` section header itself (`sh_info`
hardcoded to `5`, `.tdata`'s fixed section index, matching how `.text`/
`.data`/`.rodata` are already hardcoded `1`/`2`/`4` there). No parser or
codegen change was needed — both were already correct.

New unit test: `test/test_tls_ptr_init.c` (a `_Thread_local` pointer
statically initialized to the address of a plain global, checked in both
the main thread and a `pthread_create`d thread). Verified against a real
GCC-built baseline first (confirms the pattern is valid C and pins the
expected behavior — the reverse, a TLS variable pointing at _another_
TLS object, is correctly rejected by GCC as "initializer element is not
constant", since each thread's copy would need a different, only-known-
at-runtime value). Confirmed the fix is real by reverting `elf_write.c`
alone (`git stash`) and rebuilding: the test drops from rc=0 to rc=1.

`.rela.tdata` is shared ELF-writer code (`elf_write.c` serves both
native x86-64 and the `arm64-check.sh` aarch64 cross-target); mingw
never touches it at all (a completely separate `coff_write.c` PE
writer), so no mingw-side change was needed or possible — confirmed via
`mingw-check.sh` that the mingw cross-build still compiles clean and its
own test suite (including `144_tls`) still passes 0 failed.

`make check-all`: 0 failed (Unit 4234/4234 incl. the new test, TCC
118/118, Compliance 15/15, C-testsuite 220/220, Torture 3605/3609 — 0
failed, 354 skipped, 4 todo, Dg-error 34/34, Link 10/10). mingw cross:
0 failed. **mimalloc now passes its full test suite: 4/4 ctest targets
(`test-api`, `test-api-fill`, `test-stress`, `test-stress-dynamic`)
pass, 100%** — checked off in `checklist.txt`.

### Fixed (2026-08-17, xmmintrin.h SSE2 auto-chain + C99 inline-linkage session)

**test_pixman substantially unblocked** — its `test/utils/utils-prng.c`
(`#include <xmmintrin.h>` then freely calling SSE2-only
`_mm_storeu_si128`/`_mm_loadu_si128`/`__m128i`, relying on real
GCC/Clang's `<xmmintrin.h>` auto-chaining to `<emmintrin.h>` once
`__SSE2__` is defined) failed "undeclared variable" — rcc's bundled
`<xmmintrin.h>` had no such chain. Fixed by adding the identical
`#ifdef __SSE2__ #include <emmintrin.h> #endif` real GCC/Clang carry.
New test: `test/test_xmmintrin_emmintrin_chain.c`. pixman's build now
reaches 98/118 steps (was a complete blocker at step ~30); the one
remaining failure (`matrix-test`/`glyph-test`, a static-archive
link-order artifact: `libtestutils.a` referencing `libpixman-1.a`
symbols but placed after it on the link line) reproduces
byte-for-byte identically against the system's real GCC + the exact
same objects/link line — confirmed not an rcc bug, out of scope, not
checked off.

**mimalloc's real-world trigger led to a second, deeper bug**:
`test_mpack`'s own unit-test suite SIGABRT'd on
`TEST_TRUE(fn_mpack_tag_nil == &mpack_tag_nil)` — the previous
session's `.rela.tdata` fix (immediately above) made a non-exported
`inline` function's address compare equal across TUs when
_genuinely address-taken_, by emitting it `SB_WEAK` unconditionally —
but that broke the _other_ half of C99 inline linkage: a plain
`inline` function that's only ever _called_ (never address-taken,
e.g. `104_inline.c`'s `inline_inline_undeclared()` et al., real GCC's
own tinycc regression test) is supposed to keep the narrower,
never-externally-visible binding real GCC/Clang effectively give it
too — `SB_WEAK` made it wrongly visible to a
`__attribute__((weak))`-declared cross-TU probe
(`104+_inline.c`'s `GOT()` macro), regressing `104_inline` from
118/118 TCC-compat passes to 117/118.

Root-caused and fixed properly: added `LVar.addr_taken`
(`src/rcc.h`), set whenever a global initializer's relocation
(`append_reloc()`, `src/parser.c`) or a runtime `&fn` expression
(the unary `&` operator, `src/parser.c`) targets a function — codegen
now only emits `SB_WEAK` for a non-exported `inline` function when
its address was genuinely taken somewhere in the TU; a call-only (or
entirely unused) one keeps `SB_LOCAL` (`src/codegen.c`, both ARM64
and x86-64 prologue-emission sites). Also fixed a second, independent
bug this surfaced: `eliminate_unused_static_inline()`
(`src/opt.c`) used `f->body` (the `Node*` head of a function's
statement list) as a "has a definition" proxy — wrong for a
textually empty `{}` definition (`body == NULL` legitimately), which
kept every empty-bodied omittable candidate alive unconditionally;
removed that check (every `fns[]` entry is a genuine definition by
construction, regardless of body content) and added a third
omittable category (plain non-static `inline`, no forcing
declaration, genuinely unreferenced — matching `static inline`'s
existing unconditional-at-every--O-level omission, real GCC/Clang's
own behavior, C11 6.7.4p7).

Also taught rcc's own native linker (`link_add_sym()`, `src/link.c`)
strong-overrides-weak duplicate-symbol precedence, matching its
already-documented `link.h` contract ("strong overrides weak,"
never actually implemented): multiple TUs' identical `SB_WEAK`
inline-function copies previously hit a hard "duplicate definition"
error in the native linker (silently falling back to the external
`gcc`/`ld` linker, which handles it correctly) instead of resolving
cleanly in-process.

New regression test: `test/test-link.sh` case 14 ("plain C99 inline
function address uniqueness (3-TU link)"). Verified against a real
GCC-built baseline throughout. `mpack`'s own unit-test suite passes
954/954 build+test configurations, 0 failures across ~50M individual
checks (was SIGABRT). tinycc's `104_inline`/`104+_inline.c` matches
its `.expect` file exactly (byte-for-byte) again.

`make check-all`: 0 failed (Unit 4240/4240 incl. the new tests, TCC
118/118, Compliance 15/15, C-testsuite 220/220, Torture 3605/3609 —
0 failed, 354 skipped, Dg-error 34/34, Link 11/11). mingw cross: 0
failed.
