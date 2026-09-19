# RCC — Regoshi C Compiler

A fast, self-contained C23 (and draft C29/C2y) C compiler targeting
x86-64 on Windows, Linux and the BSDs (FreeBSD/NetBSD/OpenBSD), and
AArch64 (ARM64) on elf and darwin. Builds with either gcc or clang.
Written from scratch in C11 by Hosokawa-t, a 16 year old student. And
then ported to linux, arm64 and fixed the rest by Reini Urban.

The goal is to compile fast and correct, almost as fast as tcc, but
with full gcc compatibility and some inexpensive optimizations. We
pass much more tests than all other C compilers, fixed all the gcc
bugs gcc didn't fix yet, and esp. don't do wrong optimizations.
We do support all standards, just not the backwards incompatible ones.
I.e. C11 `int nullptr;` is wrong.

Full reference manual (every option, warning, and language
extension): [docs/rcc.md](docs/rcc.md), also available as a man page
via `make man` ([docs/rcc.pod](docs/rcc.pod)).

## Benchmark Results

Six workloads: Fibonacci(38), Ackermann(3,10), Sieve of Eratosthenes
(1M), 128×128 matrix multiply, floating-point math loop (500K), and
bubble sort (5K).

Windows:

| Compiler   | Execute (ms) | Compile (ms) | Total (ms) |
| ---------- | -----------: | -----------: | ---------: |
| RCC        |         1201 |          472 |       1673 |
| TCC 0.9.27 |         1194 |          385 |       1579 |
| GCC -O0    |         1191 |          362 |       1553 |
| GCC -O2    |         1192 |           91 |       1283 |
| CLANG -O2  |         1196 |          145 |       1341 |

Linux:

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           14 |          700 |        714 |     7% |
| RCC -O1   |           14 |          694 |        708 |     7% |
| RCC -O2   |           14 |          693 |        707 |     7% |
| TCC       |            6 |          488 |        494 |     0% |
| SLIMCC    |           34 |          498 |        532 |    67% |
| XCC       |            9 |          357 |        366 |   255% |
| KEFIR     |          183 |          571 |        754 |     6% |
| KEFIR -O1 |          191 |          306 |        497 |     1% |
| SCC       |           35 |          537 |        572 |   277% |
| LACC      |           28 |          756 |        784 |    53% |
| ANTCC     |           26 |          414 |        440 |     0% |
| CAKE      |           94 |          478 |        572 |    17% |
| BCC       |           29 |          556 |        585 |     3% |
| CCC       |           34 |          536 |        570 |    13% |
| GCC -O0   |           58 |          477 |        535 |     1% |
| GCC -O2   |          152 |          175 |        327 |    18% |
| Clang -O0 |           84 |          463 |        547 |   971% |
| Clang -O2 |          131 |          176 |        307 |    78% |

- RCC vs TCC vs GCC -O2 execution: same speed on windows, competitive on linux.
- All outputs verified correct against TCC, GCC -O2 and CLANG -O2 references.
- **Compile-time performance**: RCC has now it's own native linker,
  same as TCC. The peephole optimizer uses a 3-line sliding window
  (single pass over emitted asm), while TCC works on an internal
  abstract representation. Together these account for the compile-time
  gap. Generated code quality is on par with TCC. CCC is
  claudes-c-compiler vibe-coded in rust, which can compile the kernel.
  XCC and ANTCC are very fast, but cannot compile much.

rcc -O1 -time:

    preprocess  bench.c:   9228 us
    parse       bench.c:    629 us
    typecheck   bench.c:      4 us
    opt         bench.c:     46 us
    codegen     bench.c:    306 us
    link        bench_o1:  1295 us

### Are-We-Fast-Yet Suite

A broader, real-world-style cross-check beyond the six-workload
microbenchmark above: the full 14-benchmark ["Are We Fast
Yet?"](https://github.com/rochus-keller/Are-we-fast-yet/tree/main/C)
suite (`bench/awfy/`) — DeltaBlue (constraint solver), Richards (OS
process scheduler), Json (parser), Havlak (loop-finding dataflow
analysis), CD (collision detection), plus Bounce, List, Mandelbrot,
NBody, Permute, Queens, Sieve, Storage and Towers — covering linked
lists, tagged unions, vtable dispatch, hash maps, red-black trees,
recursive descent and floating point. Every benchmark self-verifies
its own result; all 14 pass at every optimization level with rcc.

Linux, total time (compile + best-of-3 execute) for the whole suite:

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          274 |         5822 |       6096 |     1% |
| RCC -O1   |          275 |         6542 |       6817 |     1% |
| RCC -O2   |          273 |         6060 |       6333 |     2% |
| TCC       |           65 |         5040 |       5105 |     1% |
| SLIMCC    |          249 |         4889 |       5138 |     0% |
| KEFIR     |         4363 |         5276 |       9639 |     1% |
| KEFIR -O1 |         4813 |         3669 |       8482 |     0% |
| ANTCC     |          185 |         3710 |       3895 |     1% |
| CCC       |          854 |         4466 |       5320 |     1% |
| BCC       |         2962 |         4637 |       7599 |     2% |
| GCC -O0   |          832 |         4800 |       5632 |     1% |
| GCC -O2   |         1844 |         2515 |       4359 |     1% |
| Clang -O0 |          870 |         4570 |       5440 |     0% |
| Clang -O2 |         1583 |         2543 |       4126 |     0% |

Run it yourself: `./bench/awfy/run.sh` (this benchmark alone) or
`./bench/run_bench.sh` (both tables together, plus the large-file
sqlite3.c compile-time comparison).
For more see [bench_report](bench/bench_report.md),
[bench_report_darwin](bench/bench_report_darwin.md) and
[bench_report_mingw](bench/bench_report_mingw.md).

## Test Results

Linux x86-64, combined across all test suites (TCC compatibility,
RCC unit tests, NCC compliance vs GCC, c-testsuite, GCC torture —
test/torture/, gcc-bugs), generated by
[`test-all-compilers.sh`](test-all-compilers.sh). The failure breakdown
is for all test suites, including the new gcc-bugs.
test suites with compile/runtime/dg-error granularity:
`c` = unexpected compile failure, `e` = dg-error/dg-warning test that
didn't report every expected error line, `r` = runtime failure.
Without gcc-bugs, rcc has 0 fails.

<!-- TEST_RESULTS_TABLE_START -->

| Compiler | Passed | Failed | Skipped | Notes                     |
| -------- | ------ | ------ | ------- | ------------------------- |
| rcc      | 4847   | 80     | 404     | 98%, 21c/58e/1r failures  |
| gcc      | 4845   | 244    | 246     | 95%, 96c/41e/5r failures  |
| ccc      | 4582   | 497    | 251     | 90%, 46c/86e/5r failures  |
| clang    | 4263   | 826    | 241     | 83%, 112c/40e/3r failures |
| cake     | 2395   | 2041   | 515     | 53%, 437c/3r failures     |
| compcert | 1674   | 2525   | 399     | 40%, 2336c/141r failures  |
| bcc      | 1348   | 3435   | 549     | 28%, 129c failures        |
| tcc      | 1029   | 2762   | 1526    | 27%, 116c/61r failures    |
| kefir    | 1081   | 3534   | 702     | 23%, 161c failures        |
| antcc    | 843    | 2939   | 1535    | 22%, 209c failures        |
| xcc      | 812    | 3127   | 1378    | 20%, 227c failures        |
| slimcc   | 829    | 3276   | 1212    | 20%, 207c failures        |
| lacc     | 563    | 3545   | 1209    | 13%, 294c failures        |
| scc      | 648    | 4074   | 595     | 13%, 301c failures        |
| cproc    | 362    | 4337   | 618     | 7%, 288c failures         |

<!-- TEST_RESULTS_TABLE_END -->

All compilers but rcc fail the -Whomoglyph test/test_unicode.c

## Key Features

- **Register-machine codegen** — 8-register allocator on x86-64 (r10,
  r11, rbx, r12–r15, rsi), 12-register on ARM64 (x10–x15, x19–x24)
  with dynamic allocation, no stack machine overhead. The register
  allocator is a simple first-fit bitmask with no spilling to stack
  except for the predefined spill slots. If all registers are in use,
  it spills the additional registers on the stack. Currently with a
  spill warning on -W.
- **Two-pass function emission** — Body generated to buffer first;
  prologue only pushes callee-saved registers actually used. Recursive
  functions like `fib` get zero callee-saved pushes.
- **Peephole optimizer** — Integrated inline peephole optimizer with:
  - Copy propagation (`mov r10, rax; mov r12, r10` → `mov r12, rax`)
  - Immediate folding (`mov r10, 1; add r11, r10` → `add r11, 1`)
  - Identity elimination (`mov r10, 0; add r11, r10` → deleted)
  - Strength reduction (multiply by power-of-2 → shift) in codegen already.
  - 3-instruction chain folding (`load; op; mov dst` → `load dst; op dst`)
  - Dead jump elimination (`jmp .L; .L:` → `.L:`)
  - Operates on emitted bytecode via `asm_record`/`asm_peep_try` with no separate pass
- **Switch-dispatch lowering** — `-O1`: dense small-int `switch` → jump
  table (x86); a sparse switch (no case-ranges) with >= 150 cases →
  compile-time perfect hash (32-bit multiplicative, deterministic
  bounded search, 150-256 cases, both architectures) or, above that,
  sorted binary search; fewer cases → plain linear `cmp`/`jcc` chain.
  The 150-case floor is benchmark-derived (see
  [Switch-Dispatch Lowering](#switch-dispatch-lowering-binary-search--perfect-hash)):
  below it, the tree's/hash's own less-predictable branches lose to a
  linear scan despite fewer instructions.
- **Shadow space** — Maximal 32-byte shadow space in stack frame; no `sub rsp`/`add rsp`
  per call for ≤4 args.
- **Compile-Time Function Execution (CTFE)** — AST interpreter evaluates pure functions
  with constant arguments at compile time with -O1.
- **C preprocessor** — `#include`, `#define`, `#ifdef`/`#ifndef`/`#if`, `#pragma once`,
  macro expansion with token pasting.
- **Floating-point support** — `float`/`double/long double`
  arithmetic, casts, function calls via SSE2 on x86-64 (xmm0–xmm7) or
  via ARM64 NEON/FP (v0–v7). 80-bit long double x87 on x86-64 via
  `fld`/`fstp` (truncated to 64 bits on store). ARM64 ELF 128-bit long
  double passed in register pairs (v0–v7 in even-odd pairs) following
  the AAPCS64 calling convention. Float args properly classified as
  SSE/FP class with separate GP/FP argument counters. ARM64 on APPLE
  only uses 8-byte doubles.
- **Vector/SIMD support** — x86-64 SSE/AVX and ARM64 NEON vector types
  via `__attribute__((vector_size(N)))`, with full arithmetic,
  comparison, shuffle, and broadcast operations on float and integer
  element types.
- **Windows x64 ABI** — Shadow space, correct volatile/non-volatile
  register handling, 16-byte stack alignment.
- **SystemV x64 ABI** — No Shadow space. amd64 calling convention.
  Float and struct alignment specialities.
- **ARM64 ABI (AAPCS64)** — x29 frame pointer, x30 link register,
  x0–x7 argument/return registers, x8 indirect result register, x9–x15
  caller-saved, x19–x28 callee-saved. Variadic args passed on the
  stack. 16-byte stack alignment. NEON v0–v7 for FP/SIMD args; long
  double pairs on ELF use even-odd register pairs.
- **Inline builtins** — `memset`, `memcpy`, `memcmp`, `strlen`,
  `strcmp`, `strchr` expanded inline(`rep stosb`/`rep movsb`/`repe
cmpsb`/`repne scasb`/ byte loops), avoiding libc call overhead. Also
  most other GCC/clang builtins, and `_FORTIFY_SOURCE` check
  functions. Mandatory SSE4.2 not yet.
- **Bounds checking builtins** — `__builtin_object_size` returns
  compile-time size for arrays/structs, `(size_t)-1` for pointers.
  `__builtin_dynamic_object_size` additionally reads the glibc malloc
  chunk header at runtime for heap pointers, returning the actual
  allocated size (may be larger than requested due to rounding).
  Unlike GCC -O2 which tracks malloc size through the optimizer, rcc
  reads the chunk metadata.
- **Insecure C11-C26 unicode identifier** checks, instead using true
  TR39 advised homoglyph/confusable checks via my
  [libu8indent](https://github.com/rurban/libu8ident/) library.
  Checking unicode security guidelines for identifiers.
- Simple function inliner and const-loop unroller with -O2. But no SSA
  conversion and optimizations.

### Switch-Dispatch Lowering: Binary Search / Perfect Hash

`-O1` lowers a `switch` with no case-ranges to a dense jump table (x86,
small contiguous values), else a compile-time perfect hash or sorted
binary search once there are enough cases (both architectures), else a
plain linear `cmp`/`jcc` chain — see
[docs/rcc.md#optimizations](docs/rcc.md#optimizations). The eligibility
floor (150 cases) is itself benchmark-derived, not a guess:
`bench/bench_switch.c` drives each strategy with a realistic ~90% hit
rate (dispatch mostly lands on a real case, like a typical opcode/enum
dispatch; an all-miss stream unfairly favors a trivially-predicted
linear scan) against the _old_ (pre-lowering, always-linear) codegen,
same input stream, median of 5 runs, Linux x86-64:

| Cases | Old (linear), ns/call | New, ns/call |  Delta |
| ----: | --------------------: | -----------: | -----: |
|    20 |                 32.50 |        34.68 |  -6.7% |
|    60 |                 38.25 |        40.69 |  -6.4% |
|    80 |                 38.61 |        43.51 | -12.7% |
|   100 |                 45.43 |        45.62 |  -0.4% |
|   110 |                 44.48 |        45.30 |  -1.8% |
|   128 |                 46.28 |        45.62 |  +1.4% |
|   140 |                 48.66 |        47.82 |  +1.7% |
|   150 |                 49.32 |        46.68 |  +5.4% |
|   200 |                 58.72 |        50.69 | +13.7% |
|   300 |                 81.45 |        50.58 | +37.9% |
|   500 |                113.37 |        57.11 | +49.6% |

A balanced tree's or hash's own branches are far less predictable than
a long run of heavily-biased "not this one" compares — below roughly
100-130 cases that more than offsets the fewer static instructions,
making both new strategies a measured _loss_ against the plain linear
chain, not just a wash. The win only becomes consistent (not
noise-level) from ~150 cases up, which is why `SWITCH_BSEARCH_MIN` and
`SWITCH_HASH_MIN` (`src/codegen.c`) are both set to 150 rather than the
much smaller "asymptotically better" floor a naive complexity argument
would suggest — below it, `-O1` always emits the plain chain. Perfect
hash also beat standalone binary search at every case count tested up
to hash's own 256-case search-budget ceiling, so above 150 cases binary
search's practical role is the >256-case tier and hash-construction-
failure fallback, not competing with hash below it. Run it yourself:
`rcc -O1 -o bench_switch bench/bench_switch.c && ./bench_switch`.

## Additional C Features

Computed goto (`&&label`, including label-address differences `&&a -
&&b` in `static` initializers),
`_Decimal32`/`_Decimal64`/`_Decimal128` (IEEE 754-2008 decimal
floating point via the bundled libdfp/libbid runtime), Windows and
SystemV long doubles (internally all using SSE), ARM64 long doubles
(128-bit quad precision via register pairs in elf, 8 byte on APPLE),
safe unicode identifiers and strings (unlike C11/C23), `target_clones`
(FMV with IFUNC resolver, asm .altinstr_replacements), gcc/enum/ms
bitfields, old K&R function definitions, basic -g DWARF debugging
support (line numbers only), most GCC extensions and builtins,
`_FORTIFY_SOURCE`, SIMD/NEON xmmintrin.h support, C29/C2y standard
(WG14) unconditionally, pass `-std=c2y` (or `-std=c29`).

**C contracts** — `pre(COND)`/`post([NAME:] COND)` declarator
specifiers and the `contract_assert(COND[,
"msg"])`/`contract_assume(COND[, "msg"])` statement forms, loosely
following Jens Gustedt's ["Contracts for
C"](https://gustedt.wordpress.com/2025/03/10/contracts-for-c/)
proposal ([#45](https://github.com/rurban/rcc/issues/45)). A violated
contract prints a diagnostic and `abort()`s; a literal-constant
condition is resolved at compile time like `static_assert`. At `-O3`
and above, an additional in-tree range prover statically decides
conditions from each parameter's own declared-type range (no Z3/SMT
dependency, no floating-point reasoning) that the literal fold alone
can't — see
[docs/rcc.md](docs/rcc.md#contracts-prepost-contract_assert-contract_assume)
for the full semantics.

TODO: full \_Float16/\_Float32/\_Float64/\_Float128 support (still
aliased to float/double/long double), `__STDC_IEC_60559_TYPES__` and
`__STDC_DEC_FP__` feature macros.

Unsupported (skipped in torture tests):

- **GNU nested functions with escaping function pointers** — `int
f2(...){...}` defined inside another function and passed/stored as a
  value (`g(f2)`) is supported on Linux (x86-64 and ARM64) via a
  runtime trampoline: a small per-activation stub written into the
  enclosing function's own stack frame at the point of reference,
  loading the static-chain pointer and jumping to the nested
  function's real code. The trampoline's executability relies on the
  standard `.note.GNU-stack` executable-stack ELF marking (matching
  what GCC itself emits for its own nested functions) rather than a
  runtime `mprotect` call, since libgcc's `__enable_execute_stack` is
  unreliable across real environments. Not yet implemented on Darwin
  (Mach-O) or Windows (PE) — no equivalent executable-stack wiring
  there yet. Direct calls to a nested function, and variable/`&&label`
  capture through the enclosing function's frame (static-chain
  pointer: `%r10` on x86-64, `x18` on AArch64), are supported on
  x86-64 Linux/Windows and ARM64 — including nonlocal `goto` from a
  nested function back into an enclosing `__label__`. K&R-style and
  VLA-typed nested function parameters aren't supported yet.
- **VLA struct member `offsetof`** — rcc stores VLA array members as
  fat pointers (size=16, align=8), which gives different member
  offsets than GCC's flat in-struct layout.
- `__attribute__((` **scalar_storage_order** `()))`, `__attribute__((` **mode** `()))`
- `-finstrument`, use perf instead.

Top-level `__asm__("...")` statements in AT&T, Intel or ARM syntax are
supported and emitted in source order. Unlike GCC (which hoists all
file-scope `asm` blocks to the top of the output at `-O2`/`-O3` unless
`-fno-toplevel-reorder` is used), rcc always preserves their original
position relative to functions.

The test suites pass on linux, darwin, windows, mingw-cross,
arm64-cross, darwin-cross, musl, and FreeBSD/NetBSD/OpenBSD (built
with each platform's default system `cc` — clang on FreeBSD/OpenBSD,
gcc on NetBSD — and run in CI as mandatory jobs via
[vmactions](https://github.com/vmactions), a handful of genuine,
narrow platform gaps aside — e.g. `pthread_condattr_setpshared`/
`on_exit` missing from some of these libcs — each documented at its
`is_todo_test()` gate in `run_tests.c`).
With musl the gcc torture tests fail 2 tests: c23-no-dfp-1 pr80692.

## Build

`make` or just

```bash
gcc -std=c11 -O2 -o rcc src/*.c
```

On FreeBSD/NetBSD/OpenBSD, `CC` defaults to `gcc` in the Makefile,
which isn't installed by default there — pass `CC=cc` explicitly to
use the platform's own system compiler (clang on FreeBSD/OpenBSD,
gcc on NetBSD):

```bash
gmake CC=cc
```

`LDFLAGS` is optional and additive (e.g. `LDFLAGS=-fuse-ld=lld` to
build rcc itself with LLVM's linker instead of the system default).

## Usage

```bash
# Compile to executable
./rcc -o output.exe source.c ...

# Output assembly
./rcc -S -o output.S source.c

# Run tests and benchmark
make check
make bench
```

`run_tests` supports `--parallel` (auto-detect worker count from CPU
core count) and `--jobs N`. `make check`, `make test-all`, `make
test-torture` and `make test-full` all run with `--parallel` (At least
~2x faster on the full TCC+torture+compliance+c-testsuite run). The
cross-test runners `mingw-test.sh` (under Wine), `arm64-test.sh`
(under qemu), `darwin-test.sh` (compilation only, no Mach-O execution
on Linux) also run their full suites in parallel. Since run_tests runs
now in the cross environment natively (qemu or wine), same as the
compiler and tests, it is much faster now.

## Options

Summary below; see the full reference — every option, every warning,
every `__attribute__`/`__builtin_*`/`#pragma` extension — in
[docs/rcc.md](docs/rcc.md), or as a man page via `make man`
(generates `docs/rcc.1` from [docs/rcc.pod](docs/rcc.pod)).

    -I path            add include path
    -include file      pre-include header
    -Dname[=val]       define a macro value
    -Uname             undefine a macro value
    -E                 preprocessor-only
    -S                 assemble-only
    -c                 compile-only
    -o file            set output filename
    -O0                disable peephole optimizer
    -O1                enable CTFE optimizations
    -O2                enable -finline, -funroll optimizations
    -O3                enable the contract range prover (pre/post/contract_assert/contract_assume)
    -g                 emit DWARF line-number debug info
    -std={c2y,c23,c17,c11,c99,c89,...}
                       sets __STDC_VERSION__
    -nostdinc          do not search system include directories
    -W                 print diagnostic warnings (-Wshadow, stack spilling with -v)
    -Werror            error on all warnings (but not internal stack spill warnings)
    -pedantic-errors   same
    -Wfatal-errors     exit at the first error (errors are collected otherwise)
    -fmax-errors=N     exit after N errors (default 20, 0 = unlimited)
    -Wno-homoglypth    suppress homoglyph unicode identifier warnings
    -Wno-c23-c2y-compat
                       suppress pedantic diagnostic for C2Y labeled break/continue under -std=c23
    -Wno-contract-assume-false
                       suppress the warning when contract_assume() is proven never-satisfiable
    -Wunknown-warning-option
                       for autoconf probes
    -Lpath             add linker path
    -lname             add lib
    -pthread           link with pthreads library
    -shared            create shared library
    -static            link statically
    -rdynamic          export all symbols to the dynamic symbol table (=> -Wl,-E)
    -nodefaultlibs     no libc
    -rpath path        => -Wl,-rpath,path
    -soname name       => -Wl,-soname,name
    -Wp,-MMD,file      write make dependency rules
    -MD, -MMD          write make dependency rules to a .d file
    -MF file           set the dependency output file
    -MT target         set the dependency rule target
    -MQ target         like -MT, quoting make metacharacters
    -MP                add phony targets for each prerequisite
    -Wl,<opt>          pass option to linker
    -mms-bitfields     use MSVC bitfields (default on Windows)
    -mno-ms-bitfields  use GCC bitfields (default on non-Windows)
    -pie|-fPIE|-fpie   generate position-independent executable
    -fPIC|-fpic        generate position-independent code
    -fmacro-prefix-map=old=new  remap paths in diagnostics
    -time              print timing for each compilation substep
    -v                 be more verbose
    -xc                treat input as C
    -x none            reset language input
    -###               dry-run (print commands, don't execute)
    -dM                dump all macro definitions (use with -E)
    -fdump-ast         dump AST to stderr for debugging
    -fexec-charset=cs  set execution character set (default UTF-8)
    -print-search-dirs print install, include and library paths
    --help
    --version

## Project Structure

| File                  | Description                                                                 |
| --------------------- | --------------------------------------------------------------------------- |
| `src/main.c`          | Driver: CLI, assembler/linker invocation                                    |
| `src/lexer.c`         | Tokenizer with number/string/char literal support                           |
| `src/preprocess.c`    | C preprocessor (`#include`, `#define`, `#if`, macros)                       |
| `src/parser.c`        | Recursive-descent parser → AST                                              |
| `src/type.c`          | Type system (primitives, pointers, arrays, structs, functions)              |
| `src/codegen.c`       | x86-64/ARM64 code generator with register allocator and peephole optimizer  |
| `src/cg_builtins.c`   | x86-64/ARM64 code generator for the builtins                                |
| `src/cg_vectors.c`    | x86-64/ARM64 code generator for vector support                              |
| `src/cg_opt.c`        | Cheap codegen strength reductions (constant-divisor idiv/div removal)       |
| `src/opt.c`           | AST-level optimizer and CTFE interpreter                                    |
| `src/alloc.c`         | Arena memory allocator                                                      |
| `src/unicode.{c,h}`   | libu8ident unicode identifier checks                                        |
| `src/rcc.h`           | Shared data structures and declarations                                     |
| `src/asm.{c,h}`       | Built-in assembler: parse generated `.s` text → `ObjFile` → ELF/Mach-O/COFF |
| `src/codegen_asm.h`   | Codegen ASM wrappers — emit assembled bytes directly via encoder functions  |
| `src/x86_enc.{c,h}`   | x86-64 instruction encoder (ModR/M, REX, VEX, SSE/AVX)                      |
| `src/arm64_enc.{c,h}` | ARM64 / AArch64 instruction encoder                                         |
| `src/obj.{c,h}`       | Object file representation: sections, symbols, relocations, SecBuf          |
| `src/elf_write.c`     | ELF object file writer                                                      |
| `src/macho_write.c`   | Mach-O object file writer                                                   |
| `src/coff_write.c`    | COFF object file writer                                                     |
| `src/link.{c,h}`      | Arch-independent linker functions                                           |
| `src/link_elf.c`      | ELF linker                                                                  |
| `src/link_macho.c`    | Mach-O linker                                                               |
| `src/link_pe.c`       | PE/COFF linker                                                              |
| `include/`            | Minimal C standard library headers (`stdio.h`, `math.h`, etc.)              |
| `bench/`              | Benchmark suite and runner script                                           |
| `test/`               | Test programs                                                               |

## Unix fork

The original windows repo is now at https://github.com/DocDamage/realtime-c-compiler with
[those](tcc_test_report_mingw1.1.md) test results (61/129 passed tcc tests), and [those](https://github.com/rurban/rcc/blob/old-mingw/bench/bench_report_mingw.md) benchmarks. Tested in the `old-mingw` branch via github actions.

This fork passes all tests for all architectures (x86_64 on linux and windows, aarch64 on macos) on all our testsuites. 4004/4381 GCC torture tests passed on Linux, Windows and macOS (100%). It passes much more tests than clang, gcc and all other known C compilers.

## Old Known Limitations

- **GNU Assembler (GAS) ≥2.45 on x86-64**: `call` and `jmp` to global labels in
  Intel syntax cause `operand type mismatch` errors. RCC emits `.intel_syntax noprefix`
  by default, but GAS ≥2.45 rejects direct branches to global symbols in this mode.
  Local labels (`.L.xxx`) work fine. Tests with user-defined function calls
  (`bitops-1`, `fprintf-1`, etc.) may fail to assemble under these versions.
  Root cause: rcc emits `lea r11, [rip + sI]` but GAS requires AT&T `sI(%rip)` for globals.
  Affected: most of the big torture tests with many global structs —
  20040709-1/2/3, 20071018-1, 20071030-1, 20080502-1, 20080506-1, 930106-1
  Workaround: assemble with `as --32` or use an older binutils (<2.45).
  That's why we had to switch from Intel syntax to AT&T syntax.

## License

LGPL-2.1 — see [LICENSE](LICENSE) file.
