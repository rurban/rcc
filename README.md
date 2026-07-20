# RCC — Regoshi C Compiler

A fast, self-contained C23 C compiler targeting x86-64 on Windows and
Unix, and AArch64 (ARM64) on elf and darwin. Written from scratch in
C11 by Hosokawa-t, a 16 year old student. And then ported to linux,
arm64 and fixed the rest by Reini Urban.

The goal is to be fast, almost as fast as tcc, but with full gcc
compatibility and some inexpensive optimizations. We pass much more
tests than all other C compilers, we do support all standards, just
not the backwards incompatible ones. I.e. C11 `int nullptr;` is wrong.

## Benchmark Results

Six workloads: Fibonacci(38), Ackermann(3,10), Sieve of Eratosthenes (1M), 128×128 matrix multiply, floating-point math loop (500K), and bubble sort (5K).

Windows:

| Compiler   | Execute (ms) | Compile (ms) | Total (ms) |
| ---------- | -----------: | -----------: | ---------: |
| RCC        |         1201 |          472 |       1673 |
| TCC 0.9.27 |         1194 |          385 |       1579 |
| GCC -O0    |         1191 |          362 |       1553 |
| GCC -O2    |         1192 |           91 |       1283 |
| CLANG -O2  |         1196 |          145 |       1341 |

Linux:

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           41 |          628 |        669 |
| RCC -O1   |           40 |          614 |        654 |
| RCC -O2   |           43 |          614 |        657 |
| TCC       |        **7** |          573 |    **580** |
| SLIMCC    |           51 |          630 |        681 |
| KEFIR     |          237 |          674 |        911 |
| KEFIR -O1 |          208 |      **504** |        712 |
| CCC       |           63 |          622 |        685 |
| GCC -O0   |           72 |          585 |        657 |
| GCC -O2   |          215 |          233 |        448 |
| Clang -O0 |          130 |          677 |        807 |
| Clang -O2 |          150 |          233 |        383 |

- RCC vs TCC vs GCC -O2 execution: same speed on windows, competitive on linux.
- All outputs verified correct against TCC, GCC -O2 and CLANG -O2 references.
- **Compile-time performance**: RCC invokes GCC (`system()`) to link, which is ~2× slower than TCC's native internal assembler/linker. The peephole optimizer uses a 3-line sliding window (single pass over emitted asm), while TCC works on an internal abstract representation. Together these account for the compile-time gap. Faster branch patching and a native linker as in TCC is in works. Generated code quality is on par with TCC. CCC is claudes-c-compiler vibe-coded in rust, which can compile the kernel.

rcc -O1 -time:

    preprocess  bench.c:   4215 us
    parse       bench.c:    473 us
    typecheck   bench.c:     11 us
    opt         bench.c:     21 us
    codegen     bench.c:    414 us
    link        bench_o1: 38308 us

## Key Features

- **Register-machine codegen** — 8-register allocator on x86-64 (r10, r11, rbx, r12–r15, rsi), 12-register on ARM64 (x10–x15, x19–x24) with dynamic allocation, no stack machine overhead. The register allocator is a simple first-fit bitmask with no spilling to stack except for the predefined spill slots. If all registers are in use, it spills the additional registers on the stack. Currently with a spill warning on -W.
- **Two-pass function emission** — Body generated to buffer first; prologue only pushes callee-saved registers actually used. Recursive functions like `fib` get zero callee-saved pushes.
- **Peephole optimizer** — Integrated inline peephole optimizer with:
  - Copy propagation (`mov r10, rax; mov r12, r10` → `mov r12, rax`)
  - Immediate folding (`mov r10, 1; add r11, r10` → `add r11, 1`)
  - Identity elimination (`mov r10, 0; add r11, r10` → deleted)
  - Strength reduction (multiply by power-of-2 → shift) in codegen already.
  - 3-instruction chain folding (`load; op; mov dst` → `load dst; op dst`)
  - Dead jump elimination (`jmp .L; .L:` → `.L:`)
  - Operates on emitted bytecode via `asm_record`/`asm_peep_try` with no separate pass
- **Shadow space** — Maximal 32-byte shadow space in stack frame; no `sub rsp`/`add rsp` per call for ≤4 args.
- **Compile-Time Function Execution (CTFE)** — AST interpreter evaluates pure functions with constant arguments at compile time with -O1.
- **C preprocessor** — `#include`, `#define`, `#ifdef`/`#ifndef`/`#if`, `#pragma once`, macro expansion with token pasting.
- **Floating-point support** — `float`/`double/long double` arithmetic, casts, function calls via SSE2 on x86-64 (xmm0–xmm7) or via ARM64 NEON/FP (v0–v7). 80-bit long double x87 on x86-64 via `fld`/`fstp` (truncated to 64 bits on store). ARM64 ELF 128-bit long double passed in register pairs (v0–v7 in even-odd pairs) following the AAPCS64 calling convention. Float args properly classified as SSE/FP class with separate GP/FP argument counters. ARM64 on APPLE only uses 8-byte doubles.
- **Vector/SIMD support** — x86-64 SSE/AVX and ARM64 NEON vector types via `__attribute__((vector_size(N)))`, with full arithmetic, comparison, shuffle, and broadcast operations on float and integer element types.
- **Windows x64 ABI** — Shadow space, correct volatile/non-volatile register handling, 16-byte stack alignment.
- **SystemV x64 ABI** — No Shadow space. amd64 calling convention. Float and struct alignment specialities.
- **ARM64 ABI (AAPCS64)** — x29 frame pointer, x30 link register, x0–x7 argument/return registers, x8 indirect result register, x9–x15 caller-saved, x19–x28 callee-saved. Variadic args passed on the stack. 16-byte stack alignment. NEON v0–v7 for FP/SIMD args; long double pairs on ELF use even-odd register pairs.
- **Inline builtins** — `memset`, `memcpy`, `memcmp`, `strlen`, `strcmp`, `strchr` expanded inline(`rep stosb`/`rep movsb`/`repe cmpsb`/`repne scasb`/ byte loops), avoiding libc call overhead. Also most other GCC/clang builtins, and `_FORTIFY_SOURCE` check functions. Mandatory SSE4.2 not yet.
- **Bounds checking builtins** — `__builtin_object_size` returns compile-time size for arrays/structs, `(size_t)-1` for pointers. `__builtin_dynamic_object_size` additionally reads the glibc malloc chunk header at runtime for heap pointers, returning the actual allocated size (may be larger than requested due to rounding). Unlike GCC -O2 which tracks malloc size through the optimizer, rcc reads the chunk metadata.
- **Insecure C11-C26 unicode identifier** checks, instead using true TR39 advised homoglyph/confusable checks via my [libu8indent](https://github.com/rurban/libu8ident/) library. Checking unicode security guidelines for identifiers.
- Simple function inliner and const-loop unroller with -O2.

## Supported C Features

Structs, unions, enums, typedefs, arrays (multi-dimensional), pointers (including function pointers), `for`/`while`/`do-while`/`switch`/`goto`, `sizeof`, `_Bool`, `static`, `extern`, C23 `constexpr`, `static_assert`, `nullptr`, `bool`/`true`/`false`, `[[attributes]]`, `__has_c_attribute`, `__has_include`, `<stdckdint.h>`, `0b` binary, digit separators, `u8` prefix, `__auto_type`, `__VA_OPT__`, `enum` > `int`, `#warning`/`#error`/`#elifdef`/`#elifndef`, `__attribute__((warning/error/diagnose_if))`, `__builtin_object_size`/`__builtin_dynamic_object_size`, variadic `printf`, string literals, compound assignment operators, pre/post increment, ternary operator, comma operator, designated initializers, \_Generic, attribute `__cleanup__`, `__aligned__`, `__packed__`, `__constructor__`, `__destructor__`, `vector_size`, c23 [[attribute]], Windows and SystemV long doubles (internally all using SSE), ARM64 long doubles (128-bit quad precision via register pairs in elf, 8 byte on APPLE), safe unicode identifiers and strings (unlike C11/C23), minimal `"wchar.h"`, inline, weak, gcc/enum/ms bitfields, old K&R function definitions, VLA's, atomics (LL/SC on ARM64, xadd/lock on x86), GNU alias, args... macro syntax, basic -g DWARF debugging support (line numbers only), most GCC extensions and builtins, -fpie, -fpic, TLS, int128, `_Complex`/`__complex__`, `_FORTIFY_SOURCE`, SIMD/NEON xmmintrin.h support.

TODO: trampolines, fmv, full \_Decimal/Float/Binary support
(still aliased to float), C23 `_BitInt`, `#embed`, `<stdbit.h>`, decimal float types.

Unsupported (skipped in torture tests):

- **Nested functions** (GCC extension) — function definitions inside other functions; would require trampolines on stack-executable pages.
- **Label-address differences in static initializers** (`&&lab1 - &&lab0`) — requires two-symbol ELF relocations not yet emitted.
- **VLA struct member `offsetof`** — rcc stores VLA array members as fat pointers (size=16, align=8), which gives different member offsets than GCC's flat in-struct layout.
- `__attribute__((` **scalar_storage_order** `()))`, `__attribute__((` **mode** `()))`
- `-finstrument`, use perf instead.

Top-level `__asm__("...")` statements in AT&T, Intel or ARM syntax are supported and emitted in source order. Unlike GCC (which hoists all file-scope `asm` blocks to the top of the output at `-O2`/`-O3` unless `-fno-toplevel-reorder` is used), rcc always preserves their original position relative to functions.

The test suites has all tests passed on linux, darwin, windows, mingw-cross,
arm64-cross, darwin-cross.

## Build

```bash
gcc -std=c11 -O2 -o rcc src/main.c src/lexer.c src/parser.c src/type.c src/codegen.c src/alloc.c src/preprocess.c src/opt.c
```

## Usage

```bash
# Compile to executable
./rcc.exe -o output.exe source.c ...

# Output assembly
./rcc.exe -S -o output.S source.c

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

    -I path            add include path
    -Dname[=val]       define a macro value
    -Uname             undefine a macro value
    -E                 preprocessor-only
    -S                 assemble-only
    -c                 compile-only
    -o file            set output filename
    -O0                disable peephole optimizer
    -O1                enable CTFE optimizations
    -O2                enable -finline, -funroll optimizations
    -g                 emit DWARF line-number debug info
    -std={c23,c17,c11,c99,c89,...}
                       sets __STDC_VERSION__
    -W                 print diagnostic warnings (-Wshadow, stack spilling with -v)
    -Werror            error on all warnings (but not internal stack spill warnings)
    -pedantic-errors   same
    -Wfatal-errors     exit at the first error (errors are collected otherwise)
    -fmax-errors=N     exit after N errors (default 20, 0 = unlimited)
    -Wno-homoglypth    suppress homoglyph unicode identifier warnings
    -Wunknown-warning-option
                       for autoconf probes
    -Lpath             add linker path
    -lname             add lib
    -pthread           link with pthreads library
    -shared            create shared library
    -static            link statically
    -rpath path        => -Wl,-rpath,path
    -soname name       => -Wl,-soname,name
    -Wl,<opt>          pass option to linker
    -mms-bitfields     use MSVC bitfields (default on Windows)
    -mno-ms-bitfields  use GCC bitfields (default on non-Windows)
    -pie|-fPIE|-fpie   generate position-independent executable
    -fPIC|-fpic        generate position-independent code
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
| `include/`            | Minimal C standard library headers (`stdio.h`, `math.h`, etc.)              |
| `bench/`              | Benchmark suite and runner script                                           |
| `test/`               | Test programs                                                               |

## Unix fork

The original windows repo is now at https://github.com/DocDamage/realtime-c-compiler with
[those](tcc_test_report_mingw1.1.md) test results (61/129 passed tcc tests), and [those](https://github.com/rurban/rcc/blob/old-mingw/bench/bench_report_mingw.md) benchmarks. Tested in the `old-mingw` branch via github actions.

This fork passes all tests for all architectures (x86_64 on linux and windows, aarch64 on macos) on all our testsuites. 4004/4381 GCC torture tests passed on Linux, Windows and macOS (100%). It passes much more tests than clang, gcc and all other known C compilers.

## Test Results

Linux x86-64 GCC torture suite (test/torture/), generated by
[`test-all-compilers.sh`](test-all-compilers.sh):

<!-- TEST_RESULTS_TABLE_START -->

| Compiler | Passed | Failed | Skipped | Notes                  |
| -------- | ------ | ------ | ------- | ---------------------- |
| rcc      | 4004   | 4      | 372     | 99%, 4c error fails    |
| gcc      | 3711   | 39     | 213     | 98%, 38c/1r failures   |
| ccc      | 3367   | 164    | 432     | 95%, 133c/31r failures |
| clang    | 3014   | 527    | 422     | 85%, 512c/15r failures |
| tcc      | 2168   | 381    | 1414    | 85%, 364c/17r failures |
| kefir    | 2490   | 888    | 585     | 73%, 873c/15r failures |
| slimcc   | 1555   | 1312   | 1096    | 54%, 1305c/7r failures |

<!-- TEST_RESULTS_TABLE_END -->

All compilers but rcc fail the -Whomoglyph test/test_unicode.c

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
