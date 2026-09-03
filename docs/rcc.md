# rcc reference manual

Full command-line, warning, and language-extension reference for
**rcc**, the Regoshi C Compiler. For the project overview, benchmarks
and test results, see [../README.md](../README.md). This document is
the source of truth for the `rcc.1` man page — see
[rcc.pod](rcc.pod)/`make man` — and for every rcc-specific extension
beyond standard C23/C29.

- [Synopsis](#synopsis)
- [Supported standards and dialects](#supported-standards-and-dialects)
- [Contracts (`pre`/`post`, `contract_assert`, `contract_assume`)](#contracts-prepost-contract_assert-contract_assume)
- [Options](#options)
- [Warnings and diagnostics](#warnings-and-diagnostics)
- [`__attribute__` / `[[...]]` attributes](#attribute--attributes)
- [`__builtin_*` functions](#builtin_-functions)
- [`#pragma` directives](#pragma-directives)
- [Predefined macros](#predefined-macros)
- [Environment variables](#environment-variables)
- [ABI and target notes](#abi-and-target-notes)
- [Known limitations](#known-limitations)
- [Files](#files)

## Synopsis

    rcc [options] [-o outfile] [-c|-S|-E] infile...

rcc is a from-scratch, self-contained C23 (and draft C29/C2y)
compiler for x86-64 (Windows and Unix) and AArch64/ARM64 (ELF and
Darwin), with its own integrated assembler and linker — no external
`as`/`ld` is invoked. It targets close GCC/Clang command-line and
language compatibility while compiling and linking substantially
faster, at execution speed close to `tcc`. See
[README.md#key-features](../README.md#key-features) for the codegen
and register-allocator design and benchmark numbers.

rcc is **native-only**: a single binary supports one word width/ABI.
`-m32`/`-mx32`/`-m16` are rejected with a fatal error rather than
silently miscompiling; separate cross-built binaries
(`rcc`, `rcc-arm64`, `rcc.exe`, `rcc-darwin`, `rcc-musl`) cover the
other targets (see [AGENTS.md](../AGENTS.md)).

## Supported standards and dialects

`-std=` accepts `c2y`/`gnu2y`/`c29`/`gnu29`/`iso9899:2029`,
`c23`/`gnu23`/`iso9899:2023`, `c17`/`gnu17`/`iso9899:2017`,
`c11`/`gnu11`/`iso9899:2011`, `c99`/`gnu99`/`iso9899:1999`, and
`c90`/`c89`/`gnu90`/`gnu89`/`iso9899:1990`. A `gnuNN` spelling keeps
GNU extensions enabled; a bare `cNN`/`iso9899:*` spelling additionally
defines `__STRICT_ANSI__`.

Importantly, **rcc's parser accepts the entire C23/C29 grammar
unconditionally**, regardless of `-std=`; the flag only changes what
`__STDC_VERSION__` and `__STRICT_ANSI__` report to version-gated
library headers. This is a deliberate compatibility choice: backwards
_incompatible_ changes (e.g. C23's `int nullptr;` becoming ill-formed)
are rejected regardless of `-std=`, but backwards-_compatible_
extensions are always available. C29/C2y is a moving WG14 draft with
no ratified `__STDC_VERSION__` yet; rcc reports `202500L` (matching
GCC's own placeholder; Clang instead uses `202400L`).

C29/C2y checklist items accepted unconditionally: `0o`/`0O` octal
literal prefix (with the legacy `0`-prefixed form now a pedantic
"obsolescent" warning, see [Warnings](#warnings-and-diagnostics)),
delimited universal-character-name escapes (`\u{...}`, `\N{...}`),
`_Countof`, `if`/`switch` init-declarations
(`if (int x = f(); x > 0)`), `case` ranges (`case 1 ... 3:`), labeled
`break`/`continue` on named loops, and `__COUNTER__`.

## Contracts (`pre`/`post`, `contract_assert`, `contract_assume`)

rcc accepts function contracts loosely following Jens Gustedt's
["Contracts for C"](https://gustedt.wordpress.com/2025/03/10/contracts-for-c/)
proposal — a C adaptation of C++26's contracts (`P2900`). This is an
rcc-specific extension, not part of accepted C23/C29.

A `pre(COND)` / `post([NAME:] COND)` contract-specifier trails a
function declarator's parameter list, before the `{` of a definition
or the `;` of a prototype-only declaration:

    void *my_malloc(long size) pre(size > 0) post(r: r != 0) {
        ...
    }

`pre(COND)` declares a precondition on the arguments; `post(COND)` and
`post(NAME: COND)` declare a postcondition on the function's exit,
optionally binding the return value to `NAME` for use inside `COND`.
Both accept any scalar conditional-expression (no assignment, no
top-level comma) and may repeat any number of times, in any order.

`contract_assert(COND[, "msg"])` and `contract_assume(COND[, "msg"])`
are the proposal's statement forms, used anywhere an ordinary statement
is: `contract_assert` is an always-on `assert()` (never disabled by
`NDEBUG`); `contract_assume` promises `COND` holds without checking it
— reaching it with `COND` false is undefined behavior, defined by the
proposal as `if (!(COND)) unreachable();` verbatim (rcc has no
assumption-propagating optimizer to exploit the promise for, so it
compiles to exactly that check — a real branch to
`__builtin_unreachable()`, not a no-op).

    int div10(int x) {
        contract_assert(x != 0, "divisor must not be zero");
        return 10 / x;
    }

`pre`/`post`/`contract_assert`/`contract_assume` are ordinary
identifiers, not keywords — `pre`/`post` are recognized only trailing a
function declarator's parameter list, `contract_assert`/
`contract_assume` only immediately followed by `(`, so existing code
using any of these as ordinary function or variable names is
unaffected. Because none of them need a statement or terminating token
of their own beyond an ordinary function-call syntax, the whole feature
is trivially hidden behind a feature-test macro for other compilers
(rcc predefines `__RCC__`):

    #ifndef __RCC__
    #define pre(...)
    #define post(...)
    #define contract_assert(...)
    #define contract_assume(...)
    #endif

**Semantics**: a contract on a function _definition_ is compiled once
against that definition's own parameters and (for a named `post`
binding) its return value — contracts on a separate prototype-only
declaration are not merged into a later definition; repeat them there
to enforce them. If a condition is an integer constant expression
(Gustedt's proposal draws the same line `static_assert` does), it is
discharged entirely at compile time: constant-true elides the runtime
check (for `contract_assume`, a constant-true condition is a no-op;
constant-false compiles to an unconditional `__builtin_unreachable()`,
not a compile error — the same as a bare `__builtin_unreachable()`
call), constant-false is a compile error for `pre`/`post`/
`contract_assert`. Otherwise the condition is checked at runtime — a
precondition at function entry, a postcondition at every `return` and,
for a `void` function, at the implicit fallthrough exit,
`contract_assert`/`contract_assume` inline at their own statement
position — and a violation prints a diagnostic (kind, condition text
or the message argument if given, function, file:line) to stderr and
calls `abort()`, matching `assert()`'s own failure behavior (except
`contract_assume`, whose violation is undefined behavior, not a
diagnosed abort). A `post(NAME: ...)` binding on a function returning
`void` is a compile error (there is no value to bind).

**`-O3` range prover**: `eval_const_expr`'s literal-constant fold above
runs at every optimization level; a separate, cheap, in-tree range
prover _additionally_ runs at `-O3` and above (never below — it costs
nothing at the default `-O0`/`-O1`/`-O2`). It propagates each in-scope
variable's own _declared type's_ value range (e.g. an `unsigned char`
parameter is provably in `[0,255]` regardless of what any caller
actually passes — no interprocedural/caller analysis, no SSA, no
escape/alias analysis, and deliberately no floating-point reasoning at
all: NaN/inf make "obviously true" float facts unsound without a real
IEEE-754 SMT theory) through straight-line integer arithmetic (`+ - *`,
unary `- !`, `< <= == !=`, `&& ||`, casts) to decide conditions the
literal fold alone can't:

    int g(unsigned char c) pre(c > 300) { // -O3: compile error -- no
        return c;                         // unsigned char can ever satisfy this
    }

A decided answer never changes program meaning, only whether a
no-longer-needed runtime check gets elided (provably true) or promoted
to the same compile error a literal `pre(0)` already gets (provably
false for `pre`/`post`/`contract_assert`) — except `contract_assume`,
which stays a warning (`-Wno-contract-assume-false` to suppress), not
an error, matching its constant-false case: it compiles to an
unconditional `__builtin_unreachable()`, silently discarding whatever
source follows it in the same block once codegen's existing
dead-code elision kicks in (`-O1`+).

## Options

Unrecognized flags are, by default, warned about and otherwise
tolerated (matching what real-world build systems expect from a
drop-in `cc`); `-Werror` promotes that to a hard error. An
unrecognized `-W`_name_ is always merely warned about unless
`-Werror=unknown-warning-option` is given (Clang's autoconf-probe
convention).

### Input, output and compilation mode

| Option                                     | Meaning                                                                     |
| ------------------------------------------ | --------------------------------------------------------------------------- |
| `-o file`                                  | Set the output filename.                                                    |
| `-E`                                       | Preprocess only; write to stdout (or `-o file`).                            |
| `-S`                                       | Compile only; emit assembly (`.s`).                                         |
| `-c`                                       | Compile and assemble only; do not link. Produces one `.o`/`.obj` per input. |
| `-x c`, `-x c-header`, `-xc`, `-xc-header` | Treat following inputs as C source/header regardless of extension.          |
| `-x none`                                  | Reset to extension-based language detection.                                |
| `-###`                                     | Dry run: print, don't run, assembler/linker invocations.                    |

### Preprocessor

| Option                                             | Meaning                                                                                         |
| -------------------------------------------------- | ----------------------------------------------------------------------------------------------- |
| `-I path`                                          | Add a `#include` search directory.                                                              |
| `-isystem path`, `-iquote path`, `-idirafter path` | Accepted like `-I` (system/quote-only/after search order not separately tracked).               |
| `-nostdinc`                                        | Do not search the built-in system include directories.                                          |
| `-Dname[=val]`                                     | Define a macro (default value `1`).                                                             |
| `-Uname`                                           | Undefine a macro.                                                                               |
| `-include file`                                    | Implicitly `#include` file before every translation unit's own source.                          |
| `-dM`                                              | Dump all macro definitions (use with `-E`) instead of preprocessed text.                        |
| `-fexec-charset=cs`                                | Set execution character set for string/char literal conversion (default UTF-8; requires iconv). |
| `-fmacro-prefix-map=old=new`                       | Remap a path prefix in diagnostics/`__FILE__` output (reproducible builds).                     |

### Language standard and dialect

| Option                               | Meaning                                                                                                                                    |
| ------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------ |
| `-std={c2y,c23,c17,c11,c99,c89,...}` | Sets `__STDC_VERSION__`/`__STRICT_ANSI__`; see [above](#supported-standards-and-dialects). Unrecognized value warns and falls back to C23. |
| `-funsigned-char` / `-fsigned-char`  | Make plain `char` unsigned/signed by default.                                                                                              |
| `-fwrapv`, `-fno-strict-overflow`    | Accepted no-ops (signed overflow already wraps in rcc's codegen).                                                                          |

### Warnings and error handling

See [Warnings and diagnostics](#warnings-and-diagnostics) for the message catalogue these flags gate.

| Option                                  | Meaning                                                                                                                                            |
| --------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| `-W`                                    | Enable rcc's additional (non-default) warnings.                                                                                                    |
| `-Werror`                               | Treat every warning as an error.                                                                                                                   |
| `-pedantic`, `-Wpedantic`               | Enable pedantic ISO C diagnostics.                                                                                                                 |
| `-pedantic-errors`, `--pedantic-errors` | Like `-pedantic`, and promotes _only_ those diagnostics to errors, independent of a bare `-Werror`.                                                |
| `-Wfatal-errors`                        | Stop at the first error instead of collecting up to `-fmax-errors`.                                                                                |
| `-fmax-errors=N`                        | Stop after N errors (default 20; `0` = unlimited).                                                                                                 |
| `-Werror=unknown-warning-option`        | Make an unrecognized `-Wname` a hard error (autoconf/CMake/meson warning-flag probes).                                                             |
| `-Wno-unknown-warning-option`           | Restore the default lenient handling.                                                                                                              |
| `-Wunknown-warning-option`              | No-op, accepted for autoconf probes.                                                                                                               |
| `-Wno-homoglyph`                        | Disable Unicode homoglyph/confusable identifier warnings.                                                                                          |
| `-Wno-c23-c2y-compat`                   | Suppress the pedantic C2Y labeled-`break`/`continue` diagnostic under an earlier `-std=`.                                                          |
| `-Wno-contract-assume-false`            | Suppress the warning when a `contract_assume()` is proven never-satisfiable (see [Contracts](#contracts-prepost-contract_assert-contract_assume)). |
| `-Wno-*`, `-Werror=*` (any other name)  | Silently accepted (no corresponding individually-named warning exists beyond the ones listed here).                                                |

### Optimization

| Option                                                                                     | Meaning                                                                                                             |
| ------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------- |
| `-O0`                                                                                      | Disable the peephole optimizer (default).                                                                           |
| `-O1`                                                                                      | Enable the peephole optimizer + CTFE (compile-time evaluation of pure functions called with constant arguments).    |
| `-O2`                                                                                      | `-O1` plus `-finline` (tiny-function inlining) and `-funroll` (constant-trip-count loop unrolling).                 |
| `-O3`                                                                                      | `-O2` plus the [contract range prover](#contracts-prepost-contract_assert-contract_assume). Never runs below `-O3`. |
| `-Os`, `-Ofast`, `-Og`, `-Oz`                                                              | Accepted as aliases for `-O1` (no separate size/fast/debug pipelines).                                              |
| `-finline[-functions\|-small-functions]` / `-fno-inline[-functions\|-small-functions]`     | Force-enable/disable the tiny-function inliner independent of `-O`.                                                 |
| `-funroll[-loops]` / `-fno-unroll[-loops]`                                                 | Force-enable/disable the loop unroller independent of `-O`.                                                         |
| `-fno-builtin[-name]`, `-fno-common`, `-fcommon`, `-fdata-sections`, `-ffunction-sections` | Accepted no-ops, kept for build-system compatibility.                                                               |
| `-fdefer-ts`                                                                               | Internal `thread_local` destructor-ordering compatibility flag.                                                     |

### Debugging and diagnostic output

| Option                                      | Meaning                                                                                                  |
| ------------------------------------------- | -------------------------------------------------------------------------------------------------------- |
| `-g`, `-g1`, `-g2`, `-g3`, `-ggdb[1\|2\|3]` | Emit DWARF line-number debug info (line/function mapping only — no variable locations or type info yet). |
| `-g0`                                       | Disable debug info (default).                                                                            |
| `-time`                                     | Print per-substage wall-clock timing (preprocess/parse/typecheck/opt/codegen/link) to stderr.            |
| `-v`                                        | Be more verbose (echo assembler/linker invocations; with `-W` also enables dead-code notices).           |
| `-fdump-ast`                                | Dump the parsed AST to stderr.                                                                           |
| `-print-search-dirs`                        | Print install/include/library search paths, then exit.                                                   |
| `-dumpmachine`                              | Print the target triple, then exit.                                                                      |
| `-dumpversion`                              | Print a GCC-compatible version string, then exit.                                                        |
| `--help`                                    | Print the built-in option summary, then exit.                                                            |
| `--version`                                 | Print rcc's version and target triple, then exit.                                                        |

### Code generation and ABI

| Option                                             | Meaning                                                                                                  |
| -------------------------------------------------- | -------------------------------------------------------------------------------------------------------- |
| `-mms-bitfields`                                   | MSVC-compatible bitfield layout (default on Windows).                                                    |
| `-mno-ms-bitfields`                                | GCC/System-V-compatible bitfield layout (default elsewhere).                                             |
| `-fvisibility={default,hidden,internal,protected}` | Default ELF symbol visibility (also accepts the two-argv form). Unrecognized value warns and is ignored. |
| `-m64`                                             | No-op (the x86-64 build is always 64-bit).                                                               |
| `-m32`, `-mx32`, `-m16`                            | **Fatal error** — rcc is native-only, no cross-word-width compilation in one binary.                     |
| `-pie`                                             | Generate a position-independent executable.                                                              |
| `-fPIE`, `-fpie`                                   | Generate PIE-suitable code.                                                                              |
| `-fPIC`, `-fpic`                                   | Generate shared-library-suitable position-independent code.                                              |

### Linking

| Option                              | Meaning                                                                                                                                                                                                                                      |
| ----------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `-Lpath`                            | Add a linker library search directory.                                                                                                                                                                                                       |
| `-lname`                            | Link `lib`_name_.                                                                                                                                                                                                                            |
| `-pthread`                          | Link pthreads and define its feature-test macros.                                                                                                                                                                                            |
| `-shared`                           | Produce a shared library instead of an executable.                                                                                                                                                                                           |
| `-static`                           | Link statically.                                                                                                                                                                                                                             |
| `-rdynamic`                         | Export all symbols to the dynamic symbol table (⇒ `-Wl,-E`).                                                                                                                                                                                 |
| `-nodefaultlibs`, `-nostdlib`, `-r` | Do not link default runtime libraries.                                                                                                                                                                                                       |
| `-s`                                | Strip symbols/debug info from the linker output.                                                                                                                                                                                             |
| `-rpath path`                       | ⇒ `-Wl,-rpath,path`.                                                                                                                                                                                                                         |
| `-soname name`                      | ⇒ `-Wl,-soname,name`.                                                                                                                                                                                                                        |
| `-Wl,opt[,opt...]`                  | Pass options through to the linker. Recognized sub-options include `--whole-archive`/`--no-whole-archive`, `--out-implib,path` (Windows/mingw PE import library), `-undefined,dynamic_lookup` (Darwin), `-E`, `-rpath,path`, `-soname,name`. |
| `--as-needed`, `--no-as-needed`     | No-ops.                                                                                                                                                                                                                                      |
| `-z keyword`                        | Pass a linker `-z` sub-option through.                                                                                                                                                                                                       |

### Dependency generation (Make/Ninja)

| Option                          | Meaning                                                                                                                      |
| ------------------------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| `-M`, `-MM`                     | Emit a Make dependency rule only (no compile). rcc does not distinguish system vs. user headers, so `-MM` behaves like `-M`. |
| `-MD`, `-MMD`                   | Generate a `.d` file as a side effect of compiling (same caveat re: `-MMD`).                                                 |
| `-MF file`                      | Set the dependency output file.                                                                                              |
| `-MT target`                    | Set/append the rule's target name (repeatable, space-joined).                                                                |
| `-MQ target`                    | Like `-MT` with Make-metacharacter quoting (rcc's targets never need it).                                                    |
| `-MP`                           | Add a phony target per prerequisite.                                                                                         |
| `-MG`                           | No-op (rcc always resolves includes).                                                                                        |
| `-Wp,-MD,file`, `-Wp,-MMD,file` | Equivalent to `-MD -MF file` / `-MMD -MF file` (Kbuild-style driver spelling).                                               |

## Warnings and diagnostics

rcc has two severities: **fatal errors** (`error:`, abort the
translation unit — well over a thousand distinct ISO-C
constraint/unsupported-construct checks across the lexer, parser,
type checker and code generator; these are rcc's ordinary error
surface, not individually named/toggleable) and **warnings**
(`warning:`, non-fatal unless `-Werror`/`-pedantic-errors`).

Unlike GCC/Clang, rcc does not implement individually-named
`-Wsomething` categories. `-W` is a single switch for the additional,
non-default diagnostics below.

Always active (regardless of `-W`), unless noted:

| Message                                                                                                | Condition                                                                                                                                                    | Suppress with                                                       |
| ------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------- |
| Homoglyph/confusable Unicode identifier (TR39 check via bundled libu8ident)                            | any confusable identifier                                                                                                                                    | `-Wno-homoglyph`                                                    |
| `[[attributes]] before C23 are not supported`                                                          | `[[...]]` used pre-C23 mode                                                                                                                                  | —                                                                   |
| `type defaults to int`                                                                                 | implicit-`int` declaration                                                                                                                                   | —                                                                   |
| `declaration of '%s' shadows a global declaration`                                                     | local shadows a global                                                                                                                                       | requires `-W`                                                       |
| `'%s' is static but used in inline function '%s' which is not static`                                  | ODR-risk `static` reference from a non-`static inline` function                                                                                              | —                                                                   |
| `assignment of read-only location`                                                                     | assignment through a `const`-qualified lvalue                                                                                                                | —                                                                   |
| `assignment makes integer from pointer without a cast`                                                 | pointer→integer implicit assignment                                                                                                                          | —                                                                   |
| `assignment from incompatible pointer type`                                                            | incompatible pointer-type assignment                                                                                                                         | —                                                                   |
| `pointer/integer mismatch in conditional expression`                                                   | `?:` operand type mismatch                                                                                                                                   | —                                                                   |
| `'_Alignof' applied to a function type` / `an incomplete type` / `an expression`                       | GNU `_Alignof` extension misuse                                                                                                                              | —                                                                   |
| `static_assert condition is not an integer constant expression`                                        | floating-point `static_assert` condition                                                                                                                     | —                                                                   |
| `contract_assume(%s) can never hold[...]; code after this point is unreachable and will be eliminated` | `contract_assume()` proven never-satisfiable (literal fold at any `-O`, or the [range prover](#contracts-prepost-contract_assert-contract_assume) at `-O3`+) | `-Wno-contract-assume-false`                                        |
| `octal constants with a leading zero and no 'o'/'O' are obsolescent in C2Y; use 0o... instead`         | legacy `0NNN` octal literal                                                                                                                                  | —                                                                   |
| `missing terminating "` character                                                                      | unterminated string literal (lexer recovers)                                                                                                                 | —                                                                   |
| `call to '%s': %s`                                                                                     | call to a function tagged `__attribute__((warning("...")))`                                                                                                  | — (its fatal sibling `__attribute__((error("...")))` always aborts) |
| `asm: warning: %d unresolved fixups`                                                                   | internal assembler pass                                                                                                                                      | —                                                                   |
| `rcc: link warning: unhandled relocation %u`                                                           | internal linker, unrecognized relocation type                                                                                                                | —                                                                   |
| `rcc: warning: -Wl,--out-implib,%s: %s has no exports, ...`                                            | `--out-implib` requested on a DLL with no exports                                                                                                            | —                                                                   |
| `rcc: warning: unsupported -std=%s, using C23`                                                         | unrecognized `-std=` value                                                                                                                                   | —                                                                   |
| `rcc: warning: ignored unknown visibility '%s'`                                                        | unrecognized `-fvisibility=` value                                                                                                                           | —                                                                   |
| `rcc: warning: ignored unknown option %s`                                                              | unrecognized CLI flag                                                                                                                                        | `-Werror` promotes to a hard error                                  |
| `%s:%d: warning: %s`                                                                                   | `#warning` directive text                                                                                                                                    | promoted to a fatal error+exit under bare `-Werror`                 |

Gated on `-W`:

| Message                                                          | Condition                                                                                                                                                                                  |
| ---------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `declaration of '%s' shadows a global declaration`               | see above                                                                                                                                                                                  |
| `spilling %s to stack in %s` / `spilling %s (%s) to stack in %s` | register allocator ran out of the fixed 8 (x86-64) / 12 (ARM64) general-purpose registers for a live function and used a stack spill slot instead — informational, not a correctness issue |
| `unreachable statement elided`                                   | dead-code elision after a `noreturn` call (also requires `-v`)                                                                                                                             |

Gated on `-pedantic` (promoted to an error only by `-pedantic-errors`, independent of bare `-Werror`):

| Message                                                                                     | Condition                                                             | Suppress with         |
| ------------------------------------------------------------------------------------------- | --------------------------------------------------------------------- | --------------------- |
| `ISO C does not support 'break'/'continue' statement with an identifier operand before C2Y` | named-loop `break`/`continue` used under an earlier `-std=`           | `-Wno-c23-c2y-compat` |
| `static_assert condition is not an integer`                                                 | non-constant-expression `static_assert` condition (non-floating case) | —                     |

## `__attribute__` / `[[...]]` attributes

Legacy `__attribute__((...))` GNU syntax (and its `__name__`
double-underscore alias, where both are shown below):

| Attribute           | Effect                                                                                                                      |
| ------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| `cleanup`           | Scope-exit cleanup function call.                                                                                           |
| `aligned`           | Set/raise declaration alignment.                                                                                            |
| `weak`              | Emit as a weak (`STB_WEAK`) symbol.                                                                                         |
| `visibility`        | `hidden`/`internal`/`protected`/`default` ELF visibility.                                                                   |
| `common`            | COMMON-symbol emulation (treated as `weak`).                                                                                |
| `used`              | Force emission of an otherwise-dead `static`/`static inline` symbol.                                                        |
| `always_inline`     | Force inlining regardless of `-O`.                                                                                          |
| `gnu_inline`        | GNU (pre-C99) `inline` semantics.                                                                                           |
| `ms_struct`         | MSVC bitfield layout for this struct.                                                                                       |
| `gcc_struct`        | GCC/System-V bitfield layout for this struct.                                                                               |
| `packed`            | Pack the struct/member (alignment 1).                                                                                       |
| `transparent_union` | Transparent-union call-ABI semantics.                                                                                       |
| `constructor`       | Run before `main` (`.init_array` entry).                                                                                    |
| `destructor`        | Run after `main` (`.fini_array` entry).                                                                                     |
| `alias`             | Emit as an alias to another (string-literal-named) symbol.                                                                  |
| `section`           | Place the symbol in a named section.                                                                                        |
| `target_clones`     | Multi-target function-multi-versioning (FMV) clones with an IFUNC resolver; comma-separated target list.                    |
| `target`            | Per-function target string (feature/tuning override).                                                                       |
| `warning`           | Non-fatal diagnostic at every call site (`call to '%s': %s`).                                                               |
| `error`             | Fatal diagnostic at every call site.                                                                                        |
| `diagnose_if`       | Parsed and stored; the diagnostic message is emitted unconditionally at call sites (the condition itself is not evaluated). |
| `mode`              | `QI`/`HI`/`SI`/`DI`/`TI` integer-mode override.                                                                             |
| `pure`              | Maps to rcc's "reproducible" (no-side-effect) attribute.                                                                    |
| `const`             | Maps to rcc's "unsequenced" (no-side-effect, no-memory-read) attribute.                                                     |
| `vector_size`       | SIMD vector type of the given byte size.                                                                                    |

C23 `[[...]]` syntax (a separate, only partially overlapping name set
from the legacy list above):

| Attribute                                 | Effect                                                   |
| ----------------------------------------- | -------------------------------------------------------- |
| `[[noreturn]]`                            | Function never returns.                                  |
| `[[deprecated]]`, `[[deprecated("msg")]]` | Deprecation diagnostic, with optional message.           |
| `[[reproducible]]`                        | No-side-effect (maps to legacy `pure`).                  |
| `[[unsequenced]]`                         | No-side-effect, no-memory-read (maps to legacy `const`). |

`[[gnu::packed]]`/`[[__gnu__::packed]]` namespace-qualified syntax is
accepted as an alias for legacy `packed`. `_Noreturn` (the C11/C23
_keyword_, not an attribute) is handled separately and is always
available.

`__has_attribute(name)` recognizes: `alias`, `aligned`, `cleanup`,
`const`, `constructor`, `deprecated`, `destructor`, `diagnose_if`,
`error`, `gcc_struct`, `gnu_inline`, `mode`, `ms_struct`, `noreturn`,
`packed`, `pure`, `reproducible`, `unsequenced`, `vector_size`,
`warning`, `weak` (this query list is independent of, though close to,
the implemented set above).

**Not implemented** (silently ignored — parsed and discarded, no
diagnostic): `format`, `nonnull`, `malloc`, `returns_nonnull`,
`sentinel`, `may_alias`, `artificial`, `naked`, `interrupt`,
`no_stack_protector`, `hot`, `cold`, `flatten`, `ifunc`, `optimize`,
`no_sanitize`, `fallthrough`, `unused`.

`__declspec(...)` is accepted only on Windows builds, and only as an
opaque parenthesized-content skip — no individual MS declspec keyword
(`dllexport`/`dllimport`/`novtable`/etc.) is separately recognized.

The C99 `_Pragma("string")` operator is accepted as a syntactic no-op
wherever attributes are parsed; it is consumed and discarded.

## `__builtin_*` functions

**Ordinary-call-syntax builtins** (type-checked like a normal
function call): `__builtin_abs`, `__builtin_add_overflow`,
`__builtin_add_overflow_p`, `__builtin_apply`, `__builtin_apply_args`,
`__builtin_bswap16`, `__builtin_bswap32`, `__builtin_bswap64`,
`__builtin_choose_expr`, `__builtin_classify_type`,
`__builtin_clrsb[l][l]`, `__builtin_clz[l][l]`,
`__builtin_constant_p`, `__builtin_copysign[f][l]`,
`__builtin_ctz[l][l]`, `__builtin_ffs[l][l]`,
`__builtin_fpclassify[f][l]`, `__builtin_frame_address`,
`__builtin_isinf[f][l]`, `__builtin_isfinite[f][l]`,
`__builtin_isnormal[f][l]`, `__builtin_labs`, `__builtin_llabs`,
`__builtin_longjmp`, `__builtin_memcmp`, `__builtin_memcpy`,
`__builtin_memset`, `__builtin_mul_overflow`,
`__builtin_mul_overflow_p`, `__builtin_parity[l][l]`,
`__builtin_popcount[l][l]`, `__builtin_prefetch`, `__builtin_return`,
`__builtin_return_address`, `__builtin_setjmp`,
`__builtin_signbit[f][l]`, `__builtin_strchr`, `__builtin_strcmp`,
`__builtin_strlen`, `__builtin_sub_overflow`,
`__builtin_sub_overflow_p`, `__builtin_thread_pointer`,
`__builtin_types_compatible_p`. (`__builtin_choose_expr` and
`__builtin_types_compatible_p` are resolved/const-folded at parse
time, not lowered in codegen.)

**Special-grammar builtins** (dedicated parsing, take a type or
`va_list` argument): `__builtin_offsetof`, `__builtin_va_start`,
`__builtin_va_arg`, `__builtin_va_arg_pack`,
`__builtin_va_arg_pack_len`, `__builtin_va_copy`, `__builtin_va_end`.

**Checked-arithmetic family** (sized `__builtin_{s,u}{add,sub,mul}{,l,ll}_overflow`):
`__builtin_sadd_overflow`, `__builtin_saddl_overflow`,
`__builtin_saddll_overflow`, `__builtin_uadd_overflow`,
`__builtin_uaddl_overflow`, `__builtin_uaddll_overflow`,
`__builtin_ssub_overflow`, `__builtin_ssubl_overflow`,
`__builtin_ssubll_overflow`, `__builtin_usub_overflow`,
`__builtin_usubl_overflow`, `__builtin_usubll_overflow`,
`__builtin_smul_overflow`, `__builtin_smull_overflow`,
`__builtin_smulll_overflow`, `__builtin_umul_overflow`,
`__builtin_umull_overflow`, `__builtin_umulll_overflow`.

**Recognized by name, not lexer-keyworded**: `__builtin_alloca`
(expands via `#define alloca(size) __builtin_alloca(size)`),
`__builtin_unreachable` (also drives the `-v -W` dead-code-elision
notice), the libm renames `__builtin_pow[f]`, `__builtin_fmax[f]`,
`__builtin_fmin[f]`, `__builtin_fma[f]`, `__builtin_fabs[f]`,
`__builtin_copysign[f]`, `__builtin_creal[f]`, `__builtin_cimag[f]`,
`__builtin_nextafter[f][l]` (rewritten to their non-`__builtin_` libm
names), `__builtin_cpu_supports`, `__builtin_object_size` and
`__builtin_dynamic_object_size` (see
[README.md#key-features](../README.md#key-features) for their exact
semantics — the latter reads glibc's malloc chunk header at runtime),
`__printf_chk`/`__vprintf_chk`/`__fprintf_chk`/`__vfprintf_chk`
(`_FORTIFY_SOURCE` shims). `__builtin_offsetof`, `__has_attribute`,
`__has_builtin`, `__has_include`, `__has_include_next` and
`__has_c_attribute` are handled as preprocessor pseudo-macros.

**Atomic builtins** — GCC "generic" `__atomic_*`: `__atomic_add_fetch`,
`__atomic_and_fetch`, `__atomic_clear`, `__atomic_compare_exchange[_n]`,
`__atomic_exchange[_n]`, `__atomic_fetch_add`, `__atomic_fetch_and`,
`__atomic_fetch_nand`, `__atomic_fetch_or`, `__atomic_fetch_sub`,
`__atomic_fetch_xor`, `__atomic_is_lock_free`, `__atomic_load[_n]`,
`__atomic_nand_fetch`, `__atomic_or_fetch`, `__atomic_signal_fence`,
`__atomic_store[_n]`, `__atomic_sub_fetch`, `__atomic_test_and_set`,
`__atomic_thread_fence`, `__atomic_xor_fetch` (the libatomic
`__atomic_<op>_<N>`, N=1/2/4/8/16, helper-symbol spelling is also
recognized and lowered to the same node). Legacy `__sync_*`:
`__sync_bool_compare_and_swap`, `__sync_fetch_and_add`,
`__sync_fetch_and_and`, `__sync_fetch_and_nand`,
`__sync_fetch_and_or`, `__sync_fetch_and_sub`, `__sync_fetch_and_xor`,
`__sync_lock_release`, `__sync_lock_test_and_set`,
`__sync_synchronize`, `__sync_val_compare_and_swap`.

**x86 SIMD intrinsics** — `__builtin_ia32_*`: a large name-dispatch
table covering SSE/SSE2/SSSE3/SSE4.x, the full AVX/AVX2 `*256` family,
and the AVX-512 `*512`/`*512_mask` family, plus `rdtsc`/`rdtscp` and
vector lane insert/extract. x86-only: on the ARM64 target any
`__builtin_ia32_*` name is a fatal
`__builtin_ia32_%s: not implemented on this target` error; an
unrecognized `__builtin_ia32_*` name on x86 is a fatal
`intrinsic not yet implemented` error (as opposed to the silent-ignore
behavior of unknown attributes). See
[README.md](../README.md#key-features) for the higher-level
`__attribute__((vector_size(N)))` NEON/SSE vector-type API most user
code should prefer over calling `__builtin_ia32_*` directly.

## `#pragma` directives

| Pragma                                                     | Behavior                                                                                                                 |
| ---------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| `#pragma once`                                             | Include-guard equivalent; scoped per translation unit.                                                                   |
| `#pragma pack(N)`, `pack(push[,N])`, `pack(pop)`, `pack()` | Struct packing alignment stack; bare `pack()` and `pack(0)` both reset to the compiler default.                          |
| `#pragma STDC FENV_ACCESS ON\|OFF\|DEFAULT`                | Sets the floating-point environment access mode.                                                                         |
| `#pragma push_macro("NAME")`                               | Saves NAME's current macro definition on an internal stack.                                                              |
| `#pragma pop_macro("NAME")`                                | Restores it.                                                                                                             |
| `#pragma unicode ScriptName`                               | Whitelists a Unicode script for the homoglyph/confusable-identifier checker (see [Warnings](#warnings-and-diagnostics)). |

Any other `#pragma` (e.g. `GCC diagnostic`, `GCC visibility`,
`message`, `comment`, `weak`, `pack(show)`, other `#pragma STDC ...`
forms) is tolerated and silently ignored — it is not a syntax error,
it simply has no effect.

## Predefined macros

rcc defines a GCC-compatible predefined-macro set (compiler identity
`__GNUC__`/`__VERSION__`, target triple, ABI/size macros
`__SIZEOF_*`/`__CHAR_BIT__`/`__LP64__`, every fixed-width-integer
`__<TYPE>_MAX__`/`__<TYPE>_C()` macro, IEEE-754 float/double/long
double/`_Float16/32/64/128`/`__bf16` limits, `_Decimal32/64/128`
limits, atomic lock-free feature macros, endianness macros, and ISO
conformance macros), plus rcc-specific macros:

| Macro                                                                       | Meaning                                                                                               |
| --------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------- |
| `__RCC__`                                                                   | Always `1` — rcc's self-identification macro (parallel to Clang's `__clang__`, tcc's `__TINYC__`).    |
| `__MUSL__`                                                                  | Defined when rcc itself was built against musl libc.                                                  |
| `__STDC_VERSION__`                                                          | Set from `-std=`, or left at the C23 baseline / undefined for `-std=c89`.                             |
| `__STRICT_ANSI__`                                                           | Defined only for an explicit strict (non-`gnu`) `-std=`.                                              |
| `__STDC_FENV_ACCESS__`                                                      | `1` if not already defined.                                                                           |
| `bool`, `__bool_true_false_are_defined`                                     | Macros, only under C23+ (`-std=` ≥ `202311L`).                                                        |
| `__OPTIMIZE__`                                                              | `1` when `-O1` or higher is active.                                                                   |
| `__USE_FORTIFY_LEVEL`                                                       | `0` if not already defined.                                                                           |
| `__has_include`, `__has_include_next`, `__has_c_attribute`, `__has_builtin` | Pseudo-macro markers; actual evaluation is special-cased in the preprocessor, not textual expansion.  |
| `__SSIZE_TYPE__`                                                            | `long int`.                                                                                           |
| `__COUNTER__`                                                               | Always expands to `1` (present only so `#ifdef __COUNTER__` gates pass — not a true per-use counter). |
| `__builtin_atomic_arith_add/sub/or`                                         | Redirected to `__atomic_{add,sub,or}_fetch` (libgit2/libgc compatibility).                            |
| `__APPLE__`, `__leading_underscore`, `__MACH__`                             | Only when rcc itself is host-compiled on Apple.                                                       |
| `__LLP64__`                                                                 | Only on `_WIN32` host builds.                                                                         |

## Environment variables

| Variable         | Effect                                                                                                       |
| ---------------- | ------------------------------------------------------------------------------------------------------------ |
| `RCC_KEEP_TMP`   | If set, keep temporary per-file object files instead of deleting them after a multi-file compile+link.       |
| `RCC_ASM_DEBUG`  | If set, dump the generated assembly text for every function to stderr as it is emitted.                      |
| `RCC_LINK_DEBUG` | If set, print verbose native-ELF-linker diagnostics (archive member selection, symbol resolution) to stderr. |

## ABI and target notes

- **x86-64 SystemV** (Linux/BSD/Darwin): no shadow space; standard
  amd64 calling convention; SSE2 (xmm0–xmm7) for float/double args;
  80-bit x87 long double (truncated to 64 bits on store).
- **x86-64 Windows**: 32-byte caller-allocated shadow space (elided
  for calls with ≤4 args); volatile/non-volatile register set per the
  Win64 ABI; 16-byte stack alignment; long double aliases `double`
  (SSE2).
- **ARM64 (AAPCS64)**: x29 frame pointer, x30 link register, x0–x7
  args/return, x8 indirect-result register, x9–x15 caller-saved,
  x19–x28 callee-saved, NEON v0–v7 for FP/SIMD args, variadic args on
  the stack, 16-byte stack alignment. ELF long double is a true
  128-bit quad passed in even-odd register pairs; Apple ARM64 long
  double is 8 bytes (aliases `double`).
- **Register allocator**: 8 general-purpose registers on x86-64
  (r10, r11, rbx, r12–r15, rsi), 12 on ARM64 (x10–x15, x19–x24); a
  simple first-fit bitmask allocator with no register-to-register
  spilling beyond a small set of predefined spill slots — see the
  `-W` register-spill warning above.
- **Bitfields**: GCC/System-V layout by default on non-Windows,
  MSVC layout by default on Windows; override with
  `-mms-bitfields`/`-mno-ms-bitfields` or the `ms_struct`/`gcc_struct`
  attributes.

## Known limitations

(See [README.md](../README.md#old-known-limitations) for the full,
up-to-date list, including the GNU-Assembler-≥2.45-with-Intel-syntax
workaround.) Notable unsupported constructs, intentionally skipped in
the torture-test suite rather than silently miscompiled:

- GNU nested functions with escaping function pointers: supported on
  Linux (x86-64 and ARM64) via a runtime trampoline written into the
  enclosing function's own stack frame; **not yet implemented on
  Darwin (Mach-O) or Windows (PE)**.
- VLA struct member `offsetof`: rcc stores VLA array members as fat
  pointers (size 16, align 8), giving different member offsets than
  GCC's flat in-struct layout.
- `__attribute__((scalar_storage_order(...)))`,
  `__attribute__((mode(...)))` beyond the integer-mode subset listed
  above.
- `-finstrument-functions` — use `perf` instead.
- Full `_Float16`/`_Float32`/`_Float64`/`_Float128` as distinct types
  (currently aliased to `float`/`double`/`long double`);
  `__STDC_IEC_60559_TYPES__`/`__STDC_DEC_FP__` feature macros not yet
  defined.

## Files

- `include/` — rcc's bundled minimal C standard library headers,
  installed to `$PREFIX/include/rcc` and searched automatically unless
  `-nostdinc` is given.
- `rcc_lib.so`/`rcc_lib.dll`, `lib/libdfp*` — runtime support
  libraries (the `_Decimal32/64/128` libbid/libdfp runtime, and the
  loadable-library form of rcc), installed to `$PREFIX/lib/rcc`.
- `$PREFIX/share/doc/rcc/` (Unix) / `$PREFIX/doc/` (Windows) —
  `README.md`, this file, and the test/benchmark reports, installed
  alongside the compiler.
- [`rcc.pod`](rcc.pod) — POD source for the `rcc(1)` man page;
  generate with `make man` (`pod2man docs/rcc.pod > docs/rcc.1`).
