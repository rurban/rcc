# Third-Party Test Results & Triage TODO

Batch run: 2026-08-06 (199 of 221 targets)
Binary: rcc HEAD (third_party branch)

## Summary

| rc  | count | meaning                             |
| --- | ----- | ----------------------------------- |
| 0   | 54    | pass                                |
| 2   | 101   | build/compile failure               |
| 1   | 18    | runtime/test failure                |
| 124 | 12    | timeout (420 s)                     |
| 127 | 10    | missing tool (muon, lzip, etc.)     |
| 139 | 1     | SIGSEGV (box2d C++ binary, not rcc) |
| 8   | 2     | test failure (blake3)               |
| 6   | 1     | —                                   |
| 1   | 18    | runtime/test failure (many are build-system: CC not respected) |
| 124 | 12    | timeout (420 s)                     |
| 127 | 10    | missing tool (muon, lzip, etc.)     |

## File Layout

**⚠ False positives**: Many projects (lua, mruby, most autotools/cmake projects)
hardcode `CC=gcc` in their Makefiles and ignore the environment.  The test
harness sets `CC=rcc` but the build system overrides it.  Verify by checking
`strings <binary> | grep GCC` — if it says GCC, rcc wasn't used.

**Genuine rcc bugs found so far**:

### Fixed (2026-08-06)
- **use_staging spill-preservation** — 8 VRegs live across call silently corrupted values
- **stdint.h**: missing INT_FAST*_MAX/MIN, INT_LEAST*_MAX/MIN, UINT_FAST*_MAX (24 macros)
- **preprocess**: __DATE__ / __TIME__ predefined macros missing

### Known rcc limitations (not yet fixed)
- **x86 intrinsics**: SSE/SSSE3/AVX headers (__v8hi, __builtin_shufflevector, _mm_*) — ~10+ projects
- **__VA_OPT__**: C23 variadic macro (bfs) — keyword registered but not expanded
- **_BitInt(N)**: C23 bit-precise integers (cproc, c23doku)
- **bool as macro vs keyword**: flatcc — `#define bool _Bool` breaks ## paste; should be keyword like gcc

- `test/third_party/results.txt` — tab-separated: `rc\ttest_name\tduration`
- `test/third_party/logs/test_*.log` — per-project build + test output
- `test/third_party/test_<name>/` — failing project dirs (passing dirs auto-deleted)

Re-run a single test:
CC=$(pwd)/rcc bash test/linux*thirdparty.bash test*<name>

---

## rc=1 — Runtime Failures (builds OK, test fails)

| test             | symptom                                                            |
| ---------------- | ------------------------------------------------------------------ |
| test_lua         | db.lua:83 assertion: debug.getinfo(f).short_src                    |
| test_mruby       | mrbtest binary crashes                                             |
| test_curl        | configure: "compiler does not halt on function prototype mismatch" |
| test_c23doku     | C23 \_BitInt(N) not supported                                      |
| test_c3          | CMake: missing LLD_COFF                                            |
| test_coremarkpro | benchmark runner can't find perf logs                              |
| test_box3d       | C++ binary (g++ compiled, not rcc)                                 |
| test_glib        | —                                                                  |
| test_got         | configure: missing libbsd-overlay                                  |
| test_ksh93       | —                                                                  |
| test_libgmp      | configure: cannot determine 32-bit word directive                  |
| test_muon        | muon self-tests (some pass, some fail)                             |
| test_neovim      | —                                                                  |
| test_nob         | git checkout only (build not reached?)                             |
| test_rsync       | —                                                                  |
| test_samba       | —                                                                  |
| test_scrapscript | rcc compile fails (exit 1) during Python test harness              |
| test_tcpdump     | —                                                                  |

---

## rc=2 — Build Failures (compile/link error)

Top root causes identified:

### Fixed (2026-08-06)

- `UINT_FAST64_MAX` / `INT_LEAST*_MAX` undeclared — **fixed** (stdint.h: added 24 missing limit macros)
  → unblocks: coreutils, diffutils, gpatch, gsed, gtar
- `__DATE__` / `__TIME__` undeclared — **fixed** (preprocess.c: added C89 predefined macros)
  → unblocks: mimalloc

### Needs fixing

1. **Missing x86 intrinsics** (~10+ projects)
   - `_mm_shuffle_epi8` (SSSE3) → test_bearssl, test_blosc2, test_libflac
   - `__v8hi`, `__builtin_shufflevector` (GCC vector ext) → test_blake3, test_brotli, test_ffc, test_fftw, test_libwebp, ...
   - Root: rcc can't parse GCC's `<*mmintrin.h>` headers; these use `__v8hi` types and `__builtin_ia32_*` builtins

2. **C23 `__VA_OPT__`** — test_bfs
   - `__VA_OPT__` recognized as keyword but not expanded; `## __VA_OPT__(C)` fails

3. **Prototype mismatch not diagnosed** — test_curl configure
   - `int f(int); int f(char x) {}` compiles silently; gcc errors
   - curl's configure expects compiler to halt on mismatch → configure fails

4. **C23 `_BitInt(N)`** — test_cproc, test_c23doku
   - `_BitInt(total * 3)` → "expected specific operator"

5. **lib/tempname.c pattern** (now partially fixed)
   - `SIZE_WIDTH` undeclared in test_diffutils (project-specific macro, not stdint)

6. **Object file passed as source** — test_heatshrink
   - `.os` file compiled as C source (build system issue, not rcc)

7. **flatcc keywords.h** — `tok_kw__Bool` undeclared (C99 `_Bool` keyword issue)

8. **Link failures (environment, not rcc)**: test_file, test_libgc, test_libjansson, ...
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

1. **Prototype mismatch error** (parser.c) — tiny change, unblocks curl configure
2. **flatcc `_Bool` keyword** (parser.c) — probably a token kind issue
3. **test_lua debug.getinfo** — needs minimal repro; possible codegen issue
4. **test_mruby crash** — test binary crashes; needs gdb investigation
