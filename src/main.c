// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from chibicc by Rui Ueyama.
#include "rcc.h"
#include "asm.h"
#include "codegen_asm.h"
#include "link.h"
#include "bitint_rt.h"
#include <stdarg.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#define _getpid getpid
#endif
#include <sys/stat.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>

static uint64_t now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}
#if defined(_WIN32) || defined(__MINGW32__)
/* Under Wine, CreateProcess needs the .exe extension to find gcc.exe
 * in the Wine prefix's PATH (e.g. C:\mingw64\bin\gcc.exe).  Plain
 * "gcc" resolves through Z:\ to a Linux ELF, which cannot be run. */
#define GCC_DEFAULT "gcc.exe"
#else
#define GCC_DEFAULT "gcc"
#endif

#ifndef GCC
#define GCC GCC_DEFAULT
#endif
void add_define(char *def);
void add_undef(char *name);
void dump_ast(Program *prog);

typedef struct OutPath OutPath;
struct OutPath {
    OutPath *next;
    char *path;
};
static OutPath *out_paths;

// Growable string buffer for the linker command line: libtool link lines
// (objects interleaved with -Wl flags) routinely exceed any fixed size.
static void xappendf(char **buf, size_t *len, size_t *cap, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    if (*len + n + 1 > *cap) {
        size_t ncap = *cap ? *cap : 256;
        while (*len + n + 1 > ncap)
            ncap *= 2;
        *buf = realloc(*buf, ncap);
        if (!*buf) {
            fprintf(stderr, "rcc: fatal error: out of memory\n");
            exit(1);
        }
        *cap = ncap;
    }
    va_start(ap, fmt);
    vsnprintf(*buf + *len, *cap - *len, fmt, ap);
    va_end(ap);
    *len += n;
}

OutPath *reverse(OutPath *head) {
    OutPath *prev = NULL;
    OutPath *curr = head;

    while (curr) {
        OutPath *next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }
    return prev;
}

// Returns the contents of a given file.
static char *read_file(char *path) {
    bool is_stdin = strcmp(path, "-") == 0;
    FILE *fp = is_stdin ? stdin : fopen(path, "r");
    if (!fp)
        error("cannot open %s: %m", path);

    size_t filemax = 1 << 20, size = 0;
    char *buf = malloc(filemax);
    if (!buf)
        error("out of memory reading %s", path);
    for (;;) {
        if (size + 1 >= filemax) {
            filemax *= 2;
            char *nb = realloc(buf, filemax);
            if (!nb)
                error("out of memory reading %s", path);
            buf = nb;
        }
        size_t n = fread(buf + size, 1, filemax - size - 1, fp);
        if (n == 0) break;
        size += n;
    }

    if (size == 0 || buf[size - 1] != '\n') {
        buf[size++] = '\n';
    }
    if (!is_stdin)
        fclose(fp);
    buf[size] = '\0';
    return buf;
}

#ifndef MACHINE
#define MACHINE "unknown"
#endif


// Recognize a versioned shared-library filename (libfoo.so.1.2.3): the
// dynamic linker's SONAME convention appends numeric version components
// after ".so", so strrchr(path, '.') alone (which finds the LAST dot,
// e.g. ".2") misses these entirely and rcc would try to compile the
// binary .so as C source ("invalid token \x7fELF"). Accept a bare
// ".so"/".dylib" suffix, or ".so"/".dylib" followed by one or more
// ".<digits>" version components, optionally followed by a SemVer-style
// "-<prerelease>" tag (e.g. "libnng.so.2.0.0-dev", produced verbatim by
// nng's own CMake build: NNG_ABI_VERSION embeds a "-dev"/"-rc1"-style
// NNG_PRERELEASE suffix straight into the SONAME). The prerelease tag's
// charset (alnum, '.', '-') matches SemVer 2.0's own grammar and can
// never itself look like a compilable-source extension.
static bool is_shared_lib_path(const char *path) {
    const char *so = strstr(path, ".so");
    if (!so) {
#ifdef __APPLE__
        so = strstr(path, ".dylib");
        if (!so) return false;
        so += 6;
#else
        return false;
#endif
    } else {
        so += 3;
    }
    while (*so == '.') {
        const char *p = so + 1;
        if (!isdigit((unsigned char)*p)) return false;
        while (isdigit((unsigned char)*p)) p++;
        so = p;
    }
    if (*so == '-') {
        const char *p = so + 1;
        if (!*p) return false;
        while (isalnum((unsigned char)*p) || *p == '.' || *p == '-') p++;
        so = p;
    }
    return *so == '\0';
}

// Replace the extension of filename with new_ext, matching every real
// compiler driver's `-c`/`-S` default-output-name convention: strip
// whatever the input's own trailing extension is (whatever it turns out
// to be - rcc accepts any extension as a compilable input, see the input
// classification loop above) and append new_ext, rather than special-
// casing only a handful of "recognized" source extensions. Previously
// this only stripped .c/.i/.s, so e.g. `rcc -c foo.cxx` produced
// `foo.cxx.o` instead of the conventional `foo.o` - harmless for the
// overwhelmingly common .c case, but broke any build-probe harness that
// compiles a trial `.cxx`/`.cc`/`.C`/`.cpp` file (rcc has no separate
// C++ front end and happily compiles plain-C-compatible content under
// any of those extensions) and then looks for the resulting object file
// under the name a real compiler would have produced.
// A filename with no dot at all (dot == NULL) is untouched: new_ext is
// simply appended, exactly as before.
static char *replace_ext(char *filename, char *new_ext) {
    char *dot = strrchr(filename, '.');
    if (dot)
        return format("%.*s%s", (int)(dot - filename), filename, new_ext);
    return format("%s%s", filename, new_ext);
}

// Reject a path that could break out of the double-quoted shell command
// string built for the -S debug-disassembly objdump invocation below
// (CodeQL cpp/command-line-injection, cpp/uncontrolled-process-operation):
// asm_path/tmp_obj_path are derived from -o / the input filename, both
// attacker-controllable when rcc is invoked with untrusted arguments (a
// build service, fuzzer, etc.). The command is run via system() (matching
// the rest of this driver's tool-invocation style) inside double quotes
// that must work under both POSIX sh and cmd.exe (wine/mingw) per the
// comment at its call site, so proper escaping would need two incompatible
// quoting dialects; refusing a handful of characters no legitimate path
// ever needs is simpler and strictly safer than attempting to escape them.
// `\\` is deliberately NOT in this list: inside a double-quoted bash
// string it only ever escapes a tiny set (\" \$ \\ \` \n), never
// breaks the quote, and on Windows/MinGW it's the primary path
// separator — rejecting it would break every native-Windows `-S`
// disassembly invocation (test_peep, etc.) whose temp dir path
// contains backslashes.
// `%` is also deliberately omitted: it is only a metacharacter in
// cmd.exe (variable expansion); MSYS2/MinGW system() uses sh, where
// `%` is safe inside double quotes, and Windows temp paths routinely
// reference `%USERPROFILE%`-style unexpanded environment variables.
// `^` (cmd.exe escape) likewise cannot appear in legacy 8.3 filenames
// and is only dangerous under cmd.exe, which this path never reaches
// under an MSYS2 build.
static bool path_is_shell_safe(const char *p) {
    for (const char *c = p; *c; c++)
        if (*c == '"' || *c == '`' || *c == '$' || *c == ';' ||
            *c == '|' || *c == '&' || *c == '<' || *c == '>' || *c == '\n' ||
            *c == '\r')
            return false;
    return true;
}
void help(void) {
    printf("rcc %s %s - Copyright 2026 Hosokawa-t and Reini Urban\n", VERSION, MACHINE);
    printf("Licensed under the GNU Lesser General Public License v2.1 or later\n");
    printf("rcc [options...] [-o outfile] [-c] infile(s)...\n");
    printf("Options:\n"
           "-I path             add include path\n"
           "-Dname[=val]        define a macro\n"
           "-Uname              undefine a macro\n"
           "-include file       pre-include file before main source\n"
           "-nostdinc           do not search system include directories\n"
           "-Wp,-MD,file / -Wp,-MMD,file  write Make dependency rules\n"
           "-M / -MM            print Make dependency rule only, no compile\n"
           "-MD / -MMD          write Make dependency rules to a .d file\n"
           "-MF file            set the dependency output file\n"
           "-MT target          set the dependency rule target\n"
           "-MQ target          like -MT, quoting make metacharacters\n"
           "-MP                 add phony targets for each prerequisite\n"
           "-fmacro-prefix-map=old=new  remap paths in diagnostics\n"
           "-funsigned-char     make plain char unsigned by default\n"
           "-fsigned-char       make plain char signed by default\n"
           "-E                  preprocessor-only\n"
           "-S                  assemble-only\n"
           "-c                  compile-only\n"
           "-o file             set output filename\n"
           "-O0                 disable peephole optimizer\n"
           "-O1                 enable peephole + CTFE optimizations\n"
           "-O2, -O3            -O1 plus -finline, -funroll\n"
           "-finline            inline tiny \"return EXPR;\" functions (-fno-inline to disable)\n"
           "-funroll            unroll const-sized for-loops (-fno-unroll to disable)\n"
           "-g                  emit DWARF line-number debug info\n"
           "-std={c23,c17,c11,c99,c89,...}  sets __STDC_VERSION__\n"
           "-W                  enable more compiler warnings\n"
           "-Werror             treat all warnings as errors\n"
           "-pedantic-errors    treat pedantic warnings as errors\n"
           "-Wfatal-errors      exit at the first error\n"
           "-fmax-errors=N      exit after N errors (default 20, 0 = unlimited)\n"
           "-Werror=unknown-warning-option  for autoconf probes\n"
           "-Wno-unknown-warning-option     we warn on unknown warning options by default\n"
           "-Wno-homoglyph      disable Unicode indentifer homoglyph warnings\n"
           "-Lpath              add linker path\n"
           "-lname              add lib\n"
           "-pthread            link with pthreads library\n"
           "-shared             create shared library\n"
           "-static             link statically\n"
           "-rdynamic           export all symbols to the dynamic symbol table (=> -Wl,-E)\n"
           "-nodefaultlibs      do not link default libraries (libc, libgcc, ...)\n"
           "-rpath path         => -Wl,-rpath,path\n"
           "-soname name        => -Wl,-soname,name\n"
           "-Wl,<opt>           pass option to linker\n"
           "-mms-bitfields      use MSVC bitfield layout by default\n"
           "-mno-ms-bitfields   use GCC bitfield layout by default\n"
           "-pie|-fPIE|-fpie    generate position-independent executable\n"
           "-fPIC|-fpic         generate position-independent code\n"
           "-time               print timing for each compilation substep\n"
           "-v                  be more verbose\n"
           "-xc                 treat next input as C\n"
           "-x none             reset language input\n"
           "-###                dry-run (print commands, don't execute)\n"
           "-dM                 dump all macro definitions (use with -E)\n"
           "-fdump-ast          dump AST for debugging\n"
           "-fexec-charset=cs   set execution character set (default UTF-8)\n"
           "-print-search-dirs  print install, include and library paths\n"
           "-dumpmachine        print target version\n"
           "-dumpversion        print gcc compatibility version\n"
           "--help\n"
           "--version\n");
}

bool opt_O0 = false;
bool opt_O1 = false;
bool opt_finline = false; // -finline / enabled at -O2+
bool opt_funroll = false; // -funroll / enabled at -O2+
const char *opt_std_version = "202311L"; /* rcc defaults to C23 */
bool opt_gnu_mode = false; // -std=gnu* enables GNU extensions like typeof, ({})
const char *opt_exec_charset = NULL; /* -fexec-charset=NAME (e.g. IBM1047) */
// -funsigned-char / -fsigned-char: override plain `char`'s default
// signedness (ARM64 ABI default unsigned, x86-64 default signed).
// 0 = unset (keep ty_char's arch default), 1 = force unsigned, -1 = force signed.
int opt_char_signedness = 0;
bool opt_W = false;
bool opt_Werror = false;
// Set only by the literal "-Werror" flag below, deliberately distinct
// from opt_Werror (which -pedantic-errors also sets, for promoting
// pedantic diagnostics to errors): genuine compiler diagnostics real
// GCC only promotes to errors under an explicit bare -Werror (never
// under -pedantic-errors alone, confirmed directly against gcc) --
// currently the unrecognized-flag rejection below and preprocess.c's
// #warning handling -- must gate on this instead. rcc's own
// -pedantic-errors torture/compliance tests intentionally combine it
// with real GCC flags rcc doesn't implement (-ffreestanding, -fno-asm,
// ...) and rely on those being tolerated
// (warned, not rejected) -- unlike bare -Werror, whose only realistic
// caller is a build-system capability probe (see the muon-derived
// tests) that specifically wants an unrecognized flag or diagnostic
// to fail.
bool opt_werror_flag = false;
bool opt_pedantic = false;
bool opt_Werror_unknown = false;
bool opt_Wno_homoglyph = false;
bool opt_dryrun = false;
bool opt_dM = false;
bool opt_E = false;
bool opt_fdump_ast = false;
bool opt_g = false;
bool opt_pie = false;
bool opt_pic = false;
bool opt_shared = false;
bool opt_static = false;
bool opt_export_dynamic = false;
// -r: produce a relocatable/partial-linked object (multiple .o merged,
// not a final executable) -- needs its own flag since it changes both
// mingw's automatic ".exe" output-suffix logic and the automatic -lm
// this driver otherwise appends to every link (see both use sites).
bool opt_relocatable = false;
// -Wl,--out-implib,PATH (Windows/mingw): write an import library for the
// DLL this link produces. NULL when not requested.
char *opt_out_implib = NULL;
bool opt_time = false;
bool opt_defer_ts = false;
bool opt_v = false;
bool opt_ms_bitfields =
#ifdef _WIN32
    true;
#else
    false;
#endif
;

// -nostdinc: skip system include paths
bool opt_nostdinc = false;
// Make dependency generation. Set by -Wp,-MMD,<file> (autotools/depcomp)
// and by the bare -MD/-MMD/-MF/-MT/-MQ/-MP forms (CMake/ninja pass these
// directly to the compiler). opt_depfile is the output ".d" path (-MF or
// derived from -o); opt_dep_target overrides the rule target (-MT/-MQ);
// opt_gen_deps is set by -MD/-MMD when no explicit -MF filename was given
// so write_dep_file() can derive one; opt_dep_phony adds -MP phony rules.
// opt_deps_only is set by the bare -M/-MM forms: like -E, no compilation
// happens at all — only the dependency rule is emitted (to -MF, else the
// same place -E output would go).
const char *opt_depfile = NULL;
const char *opt_dep_target = NULL;
bool opt_gen_deps = false;
bool opt_dep_phony = false;
bool opt_deps_only = false;
// -fmacro-prefix-map=old=new
const char *opt_prefix_map_old = NULL;
const char *opt_prefix_map_new = NULL;

bool sse42_available = false;

// Scan a "-Wl,a,b,c" argument's comma-separated sub-options for an exact
// match to `tok` (e.g. "-E" or "--export-dynamic"), the same way a real
// linker driver splits -Wl, before forwarding to ld.
static bool wl_has_token(const char *arg, const char *tok) {
    const char *p = arg + 4; // skip "-Wl,"
    size_t tok_len = strlen(tok);
    while (*p) {
        const char *comma = strchr(p, ',');
        size_t len = comma ? (size_t)(comma - p) : strlen(p);
        if (len == tok_len && !strncmp(p, tok, tok_len))
            return true;
        if (!comma) break;
        p = comma + 1;
    }
    return false;
}

// Scan a "-Wl,a,b,c" argument's comma-separated sub-options for `tok`
// (e.g. "--out-implib") and return the sub-option immediately following
// it (a fresh, owned copy), or the part after '=' when `tok` is spelled
// attached (`--out-implib=path`). NULL if `tok` doesn't appear, or
// appears with no following value. GNU ld accepts both `-Wl,--opt,val`
// (gcc splits on commas into two separate argv words it forwards to ld)
// and `-Wl,--opt=val`; supports both spellings here for the same reason.
static char *wl_get_value(const char *arg, const char *tok) {
    const char *p = arg + 4; // skip "-Wl,"
    size_t tok_len = strlen(tok);
    while (*p) {
        const char *comma = strchr(p, ',');
        size_t len = comma ? (size_t)(comma - p) : strlen(p);
        if (len >= tok_len && !strncmp(p, tok, tok_len)) {
            if (len == tok_len) {
                // "...,tok,value,..." -- value is the next sub-option.
                if (!comma) return NULL;
                const char *val = comma + 1;
                const char *next_comma = strchr(val, ',');
                size_t val_len = next_comma ? (size_t)(next_comma - val) : strlen(val);
                if (val_len == 0) return NULL;
                char *out = malloc(val_len + 1);
                memcpy(out, val, val_len);
                out[val_len] = '\0';
                return out;
            }
            if (p[tok_len] == '=') {
                // "...,tok=value,..."
                size_t val_len = len - tok_len - 1;
                if (val_len == 0) return NULL;
                char *out = malloc(val_len + 1);
                memcpy(out, p + tok_len + 1, val_len);
                out[val_len] = '\0';
                return out;
            }
        }
        if (!comma) break;
        p = comma + 1;
    }
    return NULL;
}

int main(int argc, char **argv) {
#ifdef __x86_64__
    // SSE4.2 runtime detection (x86_64 only)
    sse42_available = __builtin_cpu_supports("sse4.2");
#elif defined(__aarch64__)
    // ARM64 host — no SSE4.2, native ARM64 target is implicit
#elif !defined(ARCH_ARM64)
    fprintf(stderr, "rcc: unsupported host architecture\n");
    return 1;
#endif

    init_keywords();
    init_builtins();
    init_builtin_names();
    char *out_path =
#ifdef _WIN32
        "a.exe"
#else
        "a.out"
#endif
        ;
    // Input source files, one per argv element at most, so argc bounds
    // the count. A fixed 64-slot array silently DROPPED every file past
    // the 64th: sqlite's `make testfixture` links 86 sources in a single
    // invocation, sqlite3.c and tclsqlite-ex.c landed past the cap, and
    // the link died with undefined references to every sqlite3_* symbol
    // (and no diagnostic at all from the compile step).
    char **input_files = arena_alloc(sizeof(char *) * argc);
    int n_inputs = 0;
    // -include <file>: pre-include files before main source
    const char *preinclude_files[64];
    int nb_preinclude_files = 0;
    bool opt_S = false;
    bool opt_c = false;
    bool opt_o = false;
    bool opt_stdout = false; // -o - : write final output to stdout
    // Ordered linker arguments: -l/-L/-Wl flags AND object/archive inputs,
    // in argv order. Interleaving matters (-Wl,--whole-archive lib.a
    // -Wl,--no-whole-archive), so they share one buffer.
    char *libs = NULL;
    size_t libs_len = 0, libs_cap = 0;
    bool have_link_inputs = false;
#ifdef _WIN32
    xappendf(&libs, &libs_len, &libs_cap, " -lm");
#endif

    // codeql[cpp/loop-variable-changed]: deliberate ++i/i++ to consume each flag's separate-token argument (e.g. -o, -z, -D, -include); 10 sites below
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            help();
            return 0;
        }
        if (!strcmp(argv[i], "--version")) {
            printf("rcc %s %s\n", VERSION, MACHINE);
            return 0;
        }
        if (!strcmp(argv[i], "-dumpversion")) {
            puts("5"); // first gcc which made -std=c11 default
            return 0;
        }
        if (!strcmp(argv[i], "-dumpmachine")) {
            printf("%s\n", MACHINE);
            return 0;
        }
        if (!strcmp(argv[i], "-print-search-dirs")) {
            print_search_dirs(GCC);
            return 0;
        }
        if (!strcmp(argv[i], "-S")) {
            opt_S = true;
        } else if (!strcmp(argv[i], "-c")) {
            opt_c = true;
        } else if (!strcmp(argv[i], "-E")) {
            opt_E = true;
        } else if (!strcmp(argv[i], "-O0")) {
            // Optimization-level flags aren't additive: like gcc/clang, the
            // last "-On" on the command line wins outright, undoing any
            // earlier one. kbuild routinely appends a per-file "-O0" after
            // the whole-build "-O2" (e.g. crypto/jitterentropy.c's
            // CFLAGS_jitterentropy.o = -O0) specifically to keep
            // __OPTIMIZE__ from being defined; leaving opt_O1 (and the
            // inlining/unrolling it implies) sticky from the earlier -O2
            // wrongly kept __OPTIMIZE__ defined and tripped that file's own
            // "#ifdef __OPTIMIZE__ #error ..." guard.
            opt_O0 = true;
            opt_O1 = false;
            opt_finline = false;
            opt_funroll = false;
        } else if (!strcmp(argv[i], "-O1")) {
            opt_O0 = false;
            opt_O1 = true;
            opt_finline = false;
            opt_funroll = false;
        } else if (!strcmp(argv[i], "-O2") || !strcmp(argv[i], "-O3")) {
            opt_O0 = false;
            opt_O1 = true;
            opt_finline = true; // -O2 and up enable inlining
            opt_funroll = true; // -O2 and up enable unrolling
        } else if (!strcmp(argv[i], "-finline") || !strcmp(argv[i], "-finline-functions") ||
                   !strcmp(argv[i], "-finline-small-functions")) {
            opt_finline = true;
        } else if (!strcmp(argv[i], "-fno-inline") || !strcmp(argv[i], "-fno-inline-functions") ||
                   !strcmp(argv[i], "-fno-inline-small-functions")) {
            opt_finline = false;
        } else if (!strcmp(argv[i], "-funroll") || !strcmp(argv[i], "-funroll-loops")) {
            opt_funroll = true;
        } else if (!strcmp(argv[i], "-fno-unroll") || !strcmp(argv[i], "-fno-unroll-loops")) {
            opt_funroll = false;
        } else if (!strcmp(argv[i], "-fno-builtin") || !strncmp(argv[i], "-fno-builtin-", 13) ||
                   !strcmp(argv[i], "-fno-common") || !strcmp(argv[i], "-fcommon") ||
                   !strcmp(argv[i], "-fdata-sections") || !strcmp(argv[i], "-ffunction-sections")) {
            // -fno-builtin[-NAME]: disable recognizing NAME (or every
            // libc function) as a compiler builtin with known semantics.
            // rcc's own __builtin_* recognition is name-prefix-gated
            // (only literal "__builtin_..." spellings, e.g. never treats
            // a bare "memcpy" call as the builtin), so there is nothing
            // for this flag to disable -- accepted as a no-op rather than
            // falling through to the generic unknown-option path (found
            // via test_micropython, test_jemalloc, which unconditionally
            // pass -fno-builtin). -fno-common/-fcommon: controls whether
            // uninitialized globals emit as COMMON symbols or plain BSS;
            // rcc always emits plain BSS (see codegen.c/objfile.c), which
            // is -fno-common's own behavior, so -fcommon is accepted the
            // same way rather than rejected (found via test_mongoose).
            // -fdata-sections/-ffunction-sections: place each global/
            // function in its own ELF section so a later `ld --gc-
            // sections` link can drop unreferenced ones. rcc's native
            // linker never garbage-collects sections at all (every
            // symbol it emits is always kept), so per-symbol section
            // splitting has no effect to implement -- accepted as a
            // no-op rather than rejected (found via micropython's
            // `-Werror ... -fdata-sections -ffunction-sections` build,
            // which hard-failed at the very first preprocess step).
        } else if (!strcmp(argv[i], "-W")) {
            opt_W = true;
        } else if (!strcmp(argv[i], "-Werror")) {
            opt_Werror = true;
            opt_werror_flag = true;
        } else if (!strcmp(argv[i], "-Wfatal-errors")) {
            opt_Wfatal_errors = true;
        } else if (!strncmp(argv[i], "-fmax-errors=", 13)) {
            opt_fmax_errors = atoi(argv[i] + 13);
        } else if (!strcmp(argv[i], "-Wno-homoglyph")) {
            opt_Wno_homoglyph = true;
        } else if (!strcmp(argv[i], "-Werror=unknown-warning-option")) {
            opt_Werror_unknown = true;
        } else if (!strcmp(argv[i], "-Wunknown-warning-option")) {
            ; // we already warn
        } else if (!strcmp(argv[i], "-Wno-unknown-warning-option")) {
            opt_Werror_unknown = false;
        } else if (!strcmp(argv[i], "-###")) {
            opt_dryrun = true;
        } else if (!strcmp(argv[i], "-dM")) {
            opt_dM = true;
        } else if (!strcmp(argv[i], "-fdump-ast")) {
            opt_fdump_ast = true;
        } else if (!strcmp(argv[i], "-g") || !strcmp(argv[i], "-g1") ||
                   !strcmp(argv[i], "-g2") || !strcmp(argv[i], "-g3") ||
                   !strcmp(argv[i], "-ggdb") || !strcmp(argv[i], "-ggdb1") ||
                   !strcmp(argv[i], "-ggdb2") || !strcmp(argv[i], "-ggdb3")) {
            // -ggdb selects gdb-specific debug-info extensions on top of
            // plain -g; rcc's own debug info is already gdb-oriented, so
            // there is no separate format to select -- alias it to -g.
            // Without this, real-world Makefiles that unconditionally
            // pass -ggdb (found via test_rvvm, test_valkey) hit the
            // generic unknown-option path, which is a hard error whenever
            // -Werror is also active on the same command line.
            opt_g = true;
        } else if (!strcmp(argv[i], "-g0")) {
            opt_g = false;
        } else if (!strcmp(argv[i], "-Os") || !strcmp(argv[i], "-Ofast") ||
                   !strcmp(argv[i], "-Og") || !strcmp(argv[i], "-Oz")) {
            // -Os/-Ofast/-Og/-Oz: real GCC/clang optimization levels rcc has
            // no distinct pass for (rcc's own passes aren't size- or
            // fast-math-aware, and -Og's "debug-friendly" tuning is not a
            // real deoptimization rcc implements) -- alias to -O1 rather
            // than falling through to the generic unknown-option path
            // (found via test_micropython, test_mpack, which unconditionally
            // pass -Os to a subset of translation units; -Oz via busybox's
            // own `cc-option = if $CC ... -S -o /dev/null ...` Kbuild
            // probe, which tries -Oz first and silently landed on it since
            // an unrecognized flag is a warning, not a hard error -- see
            // the -Werror-gated catch-all far below).
            opt_O0 = false;
            opt_O1 = true;
            opt_finline = false;
            opt_funroll = false;
        } else if (!strcmp(argv[i], "-mms-bitfields")) {
            opt_ms_bitfields = true;
        } else if (!strcmp(argv[i], "-mno-ms-bitfields")) {
            opt_ms_bitfields = false;
        } else if (!strcmp(argv[i], "-pie") || !strcmp(argv[i], "-fPIE") ||
                   !strcmp(argv[i], "-fpie")) {
            opt_pie = true;
        } else if (!strcmp(argv[i], "-fPIC") || !strcmp(argv[i], "-fpic")) {
            opt_pic = true;
        } else if (!strcmp(argv[i], "-time")) {
            opt_time = true;
        } else if (!strcmp(argv[i], "-v")) {
            opt_v = true;
        } else if (!strncmp(argv[i], "-o", 2)) {
            char *out = argv[i] + 2;
            if (*out == '\0') {
                if (++i >= argc) {
                    fprintf(stderr, "error: missing argument for -o\n");
                    return 1;
                }
                out = argv[i];
            }
            out_path = out;
            opt_o = true;
            if (!strcmp(out_path, "-"))
                opt_stdout = true;
        } else if (!strcmp(argv[i], "-pthread")) {
            add_define("_REENTRANT");
            xappendf(&libs, &libs_len, &libs_cap, " %s", argv[i]);
        } else if (!strcmp(argv[i], "--as-needed") ||
                   !strcmp(argv[i], "--no-as-needed")) {
            xappendf(&libs, &libs_len, &libs_cap, " -Wl,%s", argv[i]);
        } else if (!strcmp(argv[i], "-z")) {
            if (++i >= argc) {
                fprintf(stderr, "error: missing argument for -z\n");
                return 1;
            }
            xappendf(&libs, &libs_len, &libs_cap, " -Wl,-z,%s", argv[i]);
        } else if (!strcmp(argv[i], "-rpath")) {
            if (++i >= argc) {
                fprintf(stderr, "error: missing argument for -rpath\n");
                return 1;
            }
            xappendf(&libs, &libs_len, &libs_cap, " -Wl,-rpath,%s", argv[i]);
        } else if (!strcmp(argv[i], "-shared")) {
            opt_shared = true;
            xappendf(&libs, &libs_len, &libs_cap, " %s", argv[i]);
        } else if (!strcmp(argv[i], "-static")) {
            opt_static = true;
            xappendf(&libs, &libs_len, &libs_cap, " %s", argv[i]);
        } else if (!strcmp(argv[i], "-rdynamic")) {
            opt_export_dynamic = true;
            xappendf(&libs, &libs_len, &libs_cap, " %s", argv[i]);
        } else if (!strncmp(argv[i], "-l", 2) || !strncmp(argv[i], "-L", 2) ||
                   !strcmp(argv[i], "-nodefaultlibs") || !strcmp(argv[i], "-nostdlib") ||
                   !strcmp(argv[i], "-r") ||
                   !strncmp(argv[i], "-Wl,", 4)) {
            // Bare "-L path" / "-l name" (path/name as its own separate
            // argv element, as opposed to the glued "-Lpath"/"-lname"
            // form) is real GCC/ld syntax that build systems commonly
            // emit from a Make variable (e.g. kefir's own "-L
            // $(LIB_DIR) -lkefir"). Without joining it here, the bare
            // "-L"/"-l" alone was forwarded to the linker with nothing
            // after it, and the actual path/name silently fell through
            // as if it were an unrelated positional input file.
            char *larg = argv[i];
            if ((!strcmp(argv[i], "-l") || !strcmp(argv[i], "-L")) && i + 1 < argc) {
                char *flag = argv[i];
                larg = format("%s%s", flag, argv[++i]);
            }
            if (!strcmp(larg, "-r"))
                opt_relocatable = true;
            if (!strncmp(larg, "-Wl,", 4) &&
                (wl_has_token(larg, "-E") || wl_has_token(larg, "--export-dynamic")))
                opt_export_dynamic = true;
            if (!strncmp(larg, "-Wl,", 4)) {
                char *implib = wl_get_value(larg, "--out-implib");
                if (implib) {
                    free(opt_out_implib);
                    opt_out_implib = implib;
                }
            }
            xappendf(&libs, &libs_len, &libs_cap, " %s", larg);
            // A bare -Wl,<opt> / -l<name> with no source or object inputs
            // is a legitimate link-only invocation (real gcc runs the
            // linker for these instead of failing with "no input files" —
            // e.g. the `-Wl,-v` version probe that build tools like muon
            // use to detect the linker type, which must print the real
            // linker's "GNU ld version" banner). Mark it as having link
            // inputs so the "no input files" fatal below doesn't fire
            // before the link step gets to run.
            if (!strncmp(larg, "-Wl,", 4) || !strncmp(larg, "-l", 2))
                have_link_inputs = true;
        } else if (!strcmp(argv[i], "-soname")) {
            if (++i >= argc) {
                fprintf(stderr, "error: missing argument for -soname\n");
                return 1;
            }
            xappendf(&libs, &libs_len, &libs_cap, " -Wl,-soname,%s", argv[i]);
        } else if (!strncmp(argv[i], "-D", 2)) {
            char *def = argv[i] + 2;
            if (*def == '\0') {
                if (++i >= argc) {
                    fprintf(stderr, "error: missing argument for -D\n");
                    return 1;
                }
                def = argv[i];
            }
            add_define(def);
        } else if (!strncmp(argv[i], "-U", 2)) {
            char *name = argv[i] + 2;
            if (*name == '\0') {
                if (++i >= argc) {
                    fprintf(stderr, "error: missing argument for -U\n");
                    return 1;
                }
                name = argv[i];
            }
            add_undef(name);
        } else if (!strncmp(argv[i], "-I", 2)) {
            char *path = argv[i] + 2;
            if (*path == '\0') {
                if (++i >= argc) {
                    fprintf(stderr, "error: missing argument for -I\n");
                    return 1;
                }
                path = argv[i];
            }
            add_include_path(path);
        } else if (!strncmp(argv[i], "-iquote", 7) || !strncmp(argv[i], "-isystem", 8) ||
                   !strncmp(argv[i], "-idirafter", 10)) {
            // GCC include-path variants, each taking a directory argument
            // (attached, e.g. -isystem/usr/foo, or as the next argv).
            // -iquote is quote-form-only ("..." includes, never <...>);
            // -isystem/-idirafter (like -I) apply to both forms, so they
            // still fold into the shared list. The argument MUST be
            // consumed: otherwise the bare directory was left to be
            // misread as an input file, e.g. noplate's `-iquote ./src`
            // produced "error: ./src: file too large".
            size_t fl = argv[i][2] == 'q' ? 7 : (argv[i][2] == 's' ? 8 : 10);
            bool is_quote = argv[i][2] == 'q';
            char *path = argv[i] + fl;
            if (*path == '\0') {
                if (++i >= argc) {
                    fprintf(stderr, "error: missing argument for %.*s\n", (int)fl, argv[i - 1]);
                    return 1;
                }
                path = argv[i];
            }
            if (is_quote)
                add_quote_include_path(path);
            else
                add_include_path(path);
        } else if (!strcmp(argv[i], "-xc") || !strcmp(argv[i], "-xc-header") ||
                   (!strcmp(argv[i], "-x") && i + 1 < argc &&
                    (!strcmp(argv[i + 1], "c") || !strcmp(argv[i + 1], "c-header")))) {
            // "c-header" (CMake's PRECOMPILE_HEADERS support, e.g. SDL3's
            // cmake_pch.h.gch) has no real PCH backend here -- rcc's own
            // -include always source-includes the named header text, never
            // consults a sibling ".gch"/".pch", so compiling the header as
            // an ordinary C translation unit and writing a normal object
            // file to the requested (unused) ".gch" path satisfies the
            // build dependency without needing serialized PCH state.
            if (!strcmp(argv[i], "-x")) i++; // skip "c"/"c-header"
        } else if (!strcmp(argv[i], "-x") && i + 1 < argc && !strcmp(argv[i + 1], "none")) {
            i++; // reset language
        } else if (!strcmp(argv[i], "-x") && i + 1 < argc) {
            fprintf(stderr, "rcc: error: unsupported -x %s, only C is supported\n", argv[i + 1]);
            return 1;
        } else if (!strncmp(argv[i], "-std=", 5)) {
            const char *std = argv[i] + 5;
            /* rcc always compiles the C23 language, but reflects the requested
             * standard in the __STDC_VERSION__ predefined macro so that library
             * headers expose the right version-gated content. */
            if (!strcmp(std, "c23") || !strcmp(std, "gnu23") || !strcmp(std, "iso9899:2023")) {
                opt_std_version = "202311L";
                if (!strncmp(std, "gnu", 3)) opt_gnu_mode = true;
            } else if (!strcmp(std, "c17") || !strcmp(std, "gnu17") || !strcmp(std, "iso9899:2017")) {
                opt_std_version = "201710L";
                if (!strncmp(std, "gnu", 3)) opt_gnu_mode = true;
            } else if (!strcmp(std, "c11") || !strcmp(std, "gnu11") || !strcmp(std, "iso9899:2011")) {
                opt_std_version = "201112L";
                if (!strncmp(std, "gnu", 3)) opt_gnu_mode = true;
            } else if (!strcmp(std, "c99") || !strcmp(std, "gnu99") || !strcmp(std, "iso9899:1999")) {
                opt_std_version = "199901L";
                if (!strncmp(std, "gnu", 3)) opt_gnu_mode = true;
            } else if (!strcmp(std, "c90") || !strcmp(std, "c89") || !strcmp(std, "gnu90") ||
                       !strcmp(std, "gnu89") || !strcmp(std, "iso9899:1990")) {
                opt_std_version = NULL;
                if (!strncmp(std, "gnu", 3)) opt_gnu_mode = true;
            } /* C90 has no __STDC_VERSION__ */
            else
                fprintf(stderr, "rcc: warning: unsupported -std=%s, using C23\n", std);
            // GCC-compatible warning-flag handling:
            //   -Wno-*  = silently ignored (no corresponding warning to disable)
            //   -Werror=* = silently ignored (error variant for warnings we don't have)
            //   others  = warn, but only error with -Werror=unknown-warning-option
            // Build systems probe supported warnings via -Werror=unknown-warning-option.
        } else if (!strcmp(argv[i], "-nostdinc")) {
            opt_nostdinc = true;
        } else if (!strncmp(argv[i], "-include", 8) && (argv[i][8] == '=' || argv[i][8] == '\0')) {
            char *path = argv[i][8] == '=' ? argv[i] + 9 : (++i < argc ? argv[i] : NULL);
            if (!path) {
                fprintf(stderr, "error: missing argument for -include\n");
                return 1;
            }
            if (nb_preinclude_files < 64)
                preinclude_files[nb_preinclude_files++] = path;
        } else if (!strncmp(argv[i], "-fmacro-prefix-map=", 19)) {
            char *val = argv[i] + 19;
            char *eq = strchr(val, '=');
            if (eq) {
                *eq = '\0';
                opt_prefix_map_old = val;
                opt_prefix_map_new = eq + 1;
            }
        } else if (!strncmp(argv[i], "-Wp,-MMD,", 9)) {
            opt_depfile = argv[i] + 9;
            opt_gen_deps = true;
        } else if (!strncmp(argv[i], "-Wp,-MD,", 8)) {
            // Same as -Wp,-MMD, for our purposes (rcc doesn't distinguish
            // system vs. non-system headers in the .d output either way).
            // Used by Linux kernel/busybox Kbuild's scripts/Makefile.host.
            opt_depfile = argv[i] + 8;
            opt_gen_deps = true;
        } else if (!strcmp(argv[i], "-M") || !strcmp(argv[i], "-MM")) {
            // Dependency-rule-only mode: like -E, no compilation/codegen/
            // link happens at all for this input; only a Make dependency
            // rule is emitted (see opt_deps_only handling in the main
            // per-file loop and print_dep_rule() in preprocess.c). -MM
            // omits system headers on real GCC; rcc doesn't track that
            // split (see -MD/-MMD below), so it behaves like -M either way.
            opt_deps_only = true;
            opt_gen_deps = true;
        } else if (!strcmp(argv[i], "-MD") || !strcmp(argv[i], "-MMD")) {
            // GCC/Clang: generate a Make .d as a side effect of compiling.
            // (-MMD omits system headers; rcc does not track that split, so
            // it lists every included header either way — over-listing only
            // forces conservative rebuilds, never stale ones.) The output
            // filename comes from a later -MF, else write_dep_file() derives
            // it from -o. CMake and ninja drive dependency scanning this way.
            opt_gen_deps = true;
        } else if (!strncmp(argv[i], "-MF", 3)) {
            char *v = argv[i][3] ? argv[i] + 3 : (++i < argc ? argv[i] : NULL);
            if (!v) {
                fprintf(stderr, "error: missing argument for -MF\n");
                return 1;
            }
            opt_depfile = v;
            opt_gen_deps = true;
        } else if (!strncmp(argv[i], "-MT", 3) || !strncmp(argv[i], "-MQ", 3)) {
            // Rule target override. Multiple -MT/-MQ accumulate, space-
            // separated, exactly like GCC. (-MQ additionally quotes make
            // metacharacters; targets here are plain object paths with no
            // such characters, so the two are treated identically.)
            char *v = argv[i][3] ? argv[i] + 3 : (++i < argc ? argv[i] : NULL);
            if (!v) {
                fprintf(stderr, "error: missing argument for %.3s\n", argv[i]);
                return 1;
            }
            if (opt_dep_target) {
                size_t n = strlen(opt_dep_target) + 1 + strlen(v) + 1;
                char *merged = arena_alloc(n);
                snprintf(merged, n, "%s %s", opt_dep_target, v);
                opt_dep_target = merged;
            } else {
                opt_dep_target = v;
            }
            opt_gen_deps = true;
        } else if (!strcmp(argv[i], "-MP")) {
            opt_dep_phony = true;
        } else if (!strcmp(argv[i], "-MG")) {
            ; // accepted, no effect (rcc always resolves includes)
        } else if (!strncmp(argv[i], "-fexec-charset=", 15)) {
            opt_exec_charset = argv[i] + 15;
        } else if (!strcmp(argv[i], "-funsigned-char")) {
            opt_char_signedness = 1; // unsigned
        } else if (!strcmp(argv[i], "-fsigned-char")) {
            opt_char_signedness = -1; // signed
        } else if (!strcmp(argv[i], "-fdefer-ts")) {
            opt_defer_ts = true;
        } else if (!strcmp(argv[i], "-fwrapv") || !strcmp(argv[i], "-fno-strict-overflow")) {
            ; // accepted, no effect: rcc's codegen never exploits signed-overflow
            // UB (no such optimization pass exists), so it is already
            // effectively -fwrapv at all times.
        } else if (!strncmp(argv[i], "-Wno-", 5) ||
                   !strncmp(argv[i], "-Werror=", 8)) {
            ; // silently ignored
        } else if (!strcmp(argv[i], "-pedantic-errors") || !strcmp(argv[i], "--pedantic-errors")) {
            opt_Werror = true;
            opt_pedantic = true;
        } else if (!strcmp(argv[i], "-pedantic") || !strcmp(argv[i], "-Wpedantic")) {
            opt_pedantic = true;
        } else if (!strcmp(argv[i], "-m64")) {
            // Accepted no-op: rcc's native target is already 64-bit on
            // every platform it supports (x86-64, ARM64, mingw64) --
            // "-m64" just confirms the default rcc already builds for.
            // Found via rpmalloc's build (samu/ninja passes it
            // unconditionally on a 64-bit host), combined with -Werror
            // promoting the otherwise-tolerated unknown flag into a hard
            // "unrecognized command-line option '-m64'" error.
            ; // no-op
        } else if (!strcmp(argv[i], "-s")) {
            // Accepted no-op: real GCC/clang pass "-s" straight to the
            // linker (strip all symbol table and relocation info from
            // the output). rcc's own native linker has no strip pass,
            // but the flag only affects binary size/debuggability, never
            // program behavior, so silently tolerating it (like -m64
            // above) is strictly safer than hard-erroring a build that
            // happens to combine it with -Werror. Found via jerryscript's
            // CMake build (`-Werror ... -s` on its doc-example link
            // step): "rcc: error: unrecognized command-line option '-s'"
            // hard-failed the whole build.
            ; // no-op
        } else if (!strcmp(argv[i], "-m32") || !strcmp(argv[i], "-mx32") ||
                   !strcmp(argv[i], "-m16")) {
            // Native-only: one binary is one word width/ABI (AGENTS.md).
            fprintf(stderr, "rcc: fatal error: %s not supported — rcc is native-only, "
                            "no cross-compilation to a different word width in one binary\n",
                    argv[i]);
            return 1;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            // Real GCC/clang always hard-error on an unrecognized
            // non-warning flag (-f.../-m.../etc.), with or without
            // -Werror — only unknown *warning* names (-W...) get the
            // lenient "warn unless -Werror=unknown-warning-option" clang
            // convention documented above (many build systems, meson
            // included, probe warning-flag support that way and expect
            // a bare -Werror to NOT itself turn that into an error).
            // Since rcc otherwise deliberately tolerates flags it
            // doesn't implement (many third-party Makefiles pass
            // compiler-specific flags unconditionally), only promote to
            // a hard error here when the caller explicitly opted in via
            // -Werror — never unconditionally, unlike real GCC — so a
            // plain, no-Werror build keeps its existing tolerance.
            bool is_warn_flag = argv[i][1] == 'W';
            if (opt_Werror_unknown || (opt_werror_flag && !is_warn_flag)) {
                fprintf(stderr, "rcc: error: unrecognized command-line option '%s'\n", argv[i]);
                return 1;
            }
            fprintf(stderr, "rcc: warning: ignored unknown option %s\n", argv[i]);
        } else {
            // Object files and libraries go directly to the linker, in
            // argv order (interleaved with -Wl flags). They are caller
            // files: never delete them like our own temp objects.
            const char *ext = strrchr(argv[i], '.');
            if ((ext && (!strcmp(ext, ".o") || !strcmp(ext, ".os") || !strcmp(ext, ".od") || !strcmp(ext, ".lo") || !strcmp(ext, ".a")
#ifdef _WIN32
                         || !strcmp(ext, ".obj") || !strcmp(ext, ".dll") || !strcmp(ext, ".lib")
#endif
                             )) ||
                is_shared_lib_path(argv[i])) {
                xappendf(&libs, &libs_len, &libs_cap, " %s", argv[i]);
                have_link_inputs = true;
            } else {
                input_files[n_inputs++] = argv[i];
            }
        }
    }

    // Allow link-only mode: object files from command line go to the linker.
    // `-v` with no inputs (`gcc -v`, no source/object args) is gcc's own
    // "print verbose configuration and exit" invocation, used verbatim by
    // countless autoconf/hand-rolled `configure` scripts to sniff whether
    // $CC is gcc-compatible (they grep this output for the substring
    // "gcc"/"clang" to decide whether to pass -fPIC, -fvisibility, etc.).
    // Answering with a plain "no input files" error makes every such probe
    // fail and silently drops those flags (see zlib-ng's SFLAGS reset).
    if (n_inputs == 0 && !have_link_inputs && opt_v) {
        fprintf(stderr, "Using built-in specs.\n");
        fprintf(stderr, "Target: %s\n", MACHINE);
        fprintf(stderr, "Configured with: rcc, a gcc-compatible C compiler\n");
        fprintf(stderr, "Thread model: posix\n");
        fprintf(stderr, "rcc version %s (gcc-compatible, __GNUC__=15) %s\n", VERSION, MACHINE);
        return 0;
    }
    if (n_inputs == 0 && !have_link_inputs) {
        fprintf(stderr, "rcc: fatal error: no input files\n");
        return 1;
    }

    // -c -o - is impossible: object files require random access (seek).
    if (opt_c && opt_stdout) {
        fprintf(stderr, "rcc: error: -c -o - is not supported (object files require seekable output)\n");
        return 1;
    }

    // -E writes preprocessed text to -o's target when one was given (real
    // cpp/gcc behavior) — pp_print_tokens() used to always target stdout,
    // silently discarding "-o some.lds" and leaving kbuild's cmd_cpp_lds_S
    // (arch/x86/entry/vdso/*/vdso64.lds, built via `$(CPP) ... -o $@ $<`)
    // with no output file at all.
    FILE *pp_out = stdout;
    if ((opt_E || opt_deps_only) && opt_o && !opt_stdout) {
        // codeql[cpp/path-injection,cpp/world-writable-file-creation]:
        // -o is the compiler's own output destination, same trust model
        // as every other compiler's -o (relies on umask, not a sandboxed
        // path prefix — there's no privilege boundary to cross here).
        pp_out = fopen(out_path, "w");
        if (!pp_out) {
            fprintf(stderr, "rcc: error: cannot open output file %s\n", out_path);
            return 1;
        }
    }

    // Apply -funsigned-char/-fsigned-char before any TU is parsed: plain
    // `char`'s signedness must be fixed for the whole compilation (it
    // affects literal typing, integer promotions and codegen throughout).
    if (opt_char_signedness != 0)
        ty_char->is_unsigned = opt_char_signedness > 0;

    // Process each input file
    for (int fi = 0; fi < n_inputs; fi++) {
        char *cur_path = input_files[fi];

        char *asm_path;
        if (opt_S) {
            asm_path = opt_o ? out_path : replace_ext(path_basename(cur_path), ".s");
        } else if (opt_c) {
            asm_path = opt_o ? out_path : replace_ext(path_basename(cur_path), ".o");
        } else {
            asm_path = format("rcc_tmp_%d_%d_%s.o", _getpid(), fi, path_basename(cur_path));
        }

        // Tokenize and Parse
        char *contents = read_file(cur_path);
        str_intern_resize(strlen(contents)); // size hash for this file

        // Single-scan: preprocess() returns the token stream directly;
        // no separate tokenize() pass needed.
        uint64_t t0 = opt_time ? now_us() : 0;
        // Wire pre-include files (-include <file>)
        for (int pi = 0; pi < nb_preinclude_files; pi++)
            add_preinclude(preinclude_files[pi]);
        // Every other C preprocessor automatically predefines __ASSEMBLER__
        // when processing a ".S"/".s" input (assembler-with-cpp mode) —
        // headers rely on it to gate C-only content (struct/enum
        // definitions, ...) away from what an assembler will ever see,
        // separately from the kernel's own, Makefile-provided
        // -D__ASSEMBLY__. Scoped to just this one file via
        // remove_cmdline_define() below, not left set for any later input
        // in a multi-file compile.
        char *dot_ext = strrchr(cur_path, '.');
        bool is_asm_input = dot_ext && (strcmp(dot_ext, ".S") == 0 || strcmp(dot_ext, ".s") == 0);
        if (is_asm_input) add_define("__ASSEMBLER__=1");
        // GAS's own "#" end-of-line comment is not preprocessor punctuation;
        // see lex_asm_cpp_mode's declaration in rcc.h.
        if (is_asm_input) lex_asm_cpp_mode = true;
        Token *tok = preprocess(cur_path, contents);
        lex_asm_cpp_mode = false;
        if (is_asm_input) remove_cmdline_define("__ASSEMBLER__");
        if (opt_time)
            fprintf(stderr, "  preprocess  %-20s: %6llu us\n", cur_path,
                    (unsigned long long)(now_us() - t0));
        // Write Make dependency file (-Wp,-MMD,<file> / -MD / -MMD, and
        // -M/-MM combined with an explicit -MF).
        write_dep_file(out_path, cur_path);

        if (opt_deps_only) {
            // -M/-MM: no compilation at all — just the dependency rule,
            // to -MF if given (already written above), else wherever -E
            // output would go (pp_out: stdout, or -o's file).
            if (!opt_depfile) print_dep_rule(pp_out, out_path, cur_path);
            continue;
        }

        if (opt_dM) {
            printf("%s", dump_macros_text());
            continue;
        }

        if (opt_E) {
            pp_print_tokens(tok, pp_out);
            continue;
        }

        // Standalone assembly file, not C source: ".S" (preprocessed, like
        // the kernel's usr/initramfs_data.S — #ifdef/#include/macros
        // already resolved above via the same preprocess() every input
        // goes through) or ".s" (raw, no preprocessing — GAS's own "#" is
        // a comment character, not a directive, so running it through the
        // C preprocessor would be wrong). Either way this is plain
        // assembly text, not C: skip parse()/typecheck/codegen entirely
        // and hand it straight to the same assembler used for inline asm.
        {
            bool is_dot_cap_s = dot_ext && strcmp(dot_ext, ".S") == 0;
            if (is_asm_input) {
                if (opt_dryrun) continue;
                char *asm_text = is_dot_cap_s ? pp_tokens_to_text(tok) : contents;
                ObjFile obj;
                objfile_init(&obj);
                if (assemble_inline(&obj, asm_text, NULL, NULL) != 0) {
                    fprintf(stderr, "rcc: error: failed to assemble %s\n", cur_path);
                    return 1;
                }
                int wr;
#ifdef _WIN32
                wr = coff_write(&obj, asm_path);
#elif __APPLE__
                wr = macho_write(&obj, asm_path);
#else
                wr = elf_write(&obj, asm_path);
#endif
                if (wr != 0) {
                    fprintf(stderr, "rcc: error: cannot write object file %s\n", asm_path);
                    return 1;
                }
                objfile_free(&obj);
                if (!opt_S) {
                    OutPath *p = arena_alloc(sizeof(OutPath));
                    p->path = asm_path;
                    p->next = out_paths;
                    out_paths = p;
                }
                continue;
            }
        }

        t0 = opt_time ? now_us() : 0;
        Program *prog = parse(tok);
        prog->in_path = cur_path;
        if (opt_time)
            fprintf(stderr, "  parse       %-20s: %6llu us\n", cur_path,
                    (unsigned long long)(now_us() - t0));

        if (opt_fdump_ast)
            dump_ast(prog);

        // Parse errors were collected (GH #34): the AST is incomplete, so
        // skip typecheck/codegen for this file; more inputs may still be
        // parsed for their diagnostics. Failure exit happens after the loop.
        if (error_count)
            continue;

        // Self-host the wide-_BitInt runtime into this TU: when the parsed
        // source uses a _BitInt(N) with N > 64, gen_bitint emits calls to
        // the rcc_bitint_* helpers (see src/bitint_rt.c). Those functions
        // are compiled here by rcc itself (self-hosted), as `static` copies
        // private to this TU, so:
        //   - no link-time dependency on a bundled runtime object (which
        //     would need a per-target .a and a driver change);
        //   - target-correct on x86-64, ARM64 and mingw alike (rcc compiles
        //     the runtime for whatever target it is building);
        //   - multi-TU links never collide (each TU's copy is local);
        //   - -c/-S/-E modes carry the helpers inside the single .o.
        // The DCE pass (eliminate_unused_static_inline) must keep them: it
        // only sees AST-level references, not the raw calls gen_bitint
        // emits, so each injected function is marked is_used. The runtime's
        // own items join BEFORE the typecheck loop below so they are fully
        // checked like the user's code.
        if (parser_used_wide_bitint) {
            Token *rt_tok = preprocess("<rcc-bitint-runtime>", (char *)bitint_rt_src);
            Program *rt_prog = parse(rt_tok);
            for (TLItem *item = rt_prog->items; item; item = item->next) {
                if (item->kind == TL_FUNC) {
                    item->fn->is_used = true;
                    item->fn->is_static = true;
                }
            }
            // Append the runtime's items to this TU's Program so codegen
            // emits them into the same object.
            TLItem **tail = &prog->items;
            while (*tail)
                tail = &(*tail)->next;
            *tail = rt_prog->items;
            // The runtime's own string literals/globals must join the TU's
            // too (codegen walks prog->strs / prog->globals independently
            // of items).
            if (rt_prog->strs) {
                StrLit **st = &prog->strs;
                while (*st)
                    st = &(*st)->next;
                *st = rt_prog->strs;
            }
        }

        // Type system / Semantic checks
        t0 = opt_time ? now_us() : 0;
        for (TLItem *item = prog->items; item; item = item->next) {
            if (item->kind != TL_FUNC)
                continue;
            for (Node *n = item->fn->body; n; n = n->next) {
                check_type(n);
            }
        }
        if (opt_time)
            fprintf(stderr, "  typecheck   %-20s: %6llu us\n", cur_path,
                    (unsigned long long)(now_us() - t0));

        // CTFE runs only with -O1; peephole skipped with -O0.
        if (opt_O1 || opt_finline || opt_funroll) {
            t0 = opt_time ? now_us() : 0;
            optimize(prog);
            if (opt_time)
                fprintf(stderr, "  opt         %-20s: %6llu us\n", cur_path,
                        (unsigned long long)(now_us() - t0));
        }

        // Not gated on -O1: omitting a never-referenced `static inline`
        // function's body is standard-permitted (C11 6.7.4p7) and real
        // GCC/Clang do it unconditionally, not just as an optimization.
        eliminate_unused_static_inline(prog);

        if (!opt_dryrun) {
            t0 = opt_time ? now_us() : 0;
            struct ObjFile *obj = codegen(prog);
            if (opt_time) {
                fprintf(stderr, "  codegen     %-20s: %6llu us\n", cur_path,
                        (unsigned long long)(now_us() - t0));
            }
            // Write binary .o file
            // A scratch object file, disassembled below to produce the
            // ".s" text and then discarded. Named from pid+fi in the CWD
            // rather than as "<asm_path>.tmp.o": asm_path is caller-
            // controlled (-o) and may be a device node or a read-only
            // directory (e.g. Kbuild's cc-option probe uses "-S -o
            // /dev/null"), where appending ".tmp.o" to it is not a
            // writable path at all.
            char *tmp_obj_path = asm_path;
            if (opt_S) {
                tmp_obj_path = format("rcc_tmp_%d_%d.tmp.o", _getpid(), fi);
            }
            int wr;
#ifdef _WIN32
            wr = coff_write(obj, tmp_obj_path);
#elif __APPLE__
            wr = macho_write(obj, tmp_obj_path);
#else
            // codeql[cpp/path-injection]: tmp_obj_path is the compiler's
            // own object-file output path (see the -o comment above).
            wr = elf_write(obj, tmp_obj_path);
#endif
            if (wr != 0) {
                fprintf(stderr, "rcc: error: cannot write object file %s\n", tmp_obj_path);
                return 1;
            }
            objfile_free(obj);
            if (opt_S) {
                char cmd[2048];
                // Derive objdump name from GCC: "gcc" -> "objdump",
                // "aarch64-linux-gnu-gcc" -> "aarch64-linux-gnu-objdump"
                const char *objdump = "objdump";
                size_t gcc_len = strlen(GCC);
                if (gcc_len > 4 && GCC[gcc_len - 1] == 'c' && GCC[gcc_len - 2] == 'c' && GCC[gcc_len - 3] == 'g' && GCC[gcc_len - 4] == '-') {
                    // Cross-compiler: strip trailing "-gcc", append "-objdump"
                    char *triple = malloc(gcc_len - 3);
                    if (triple) {
                        assert(gcc_len > 4);
                        memcpy(triple, GCC, gcc_len - 4);
                        triple[gcc_len - 4] = '\0';
                        size_t len = strlen(triple) + 9;
                        char *xobj = malloc(len);
                        if (xobj) {
                            snprintf(xobj, len, "%s-objdump", triple);
                            objdump = xobj;
                        }
                        free(triple);
                    }
                }
                // CodeQL cpp/command-line-injection, cpp/uncontrolled-
                // process-operation: tmp_obj_path/asm_path (from -o / the
                // input filename) are embedded in a system() command
                // string below — reject anything that could break out of
                // the double quotes instead of attempting to escape it
                // (see path_is_shell_safe()'s doc comment for why).
                if (!path_is_shell_safe(tmp_obj_path) || !path_is_shell_safe(asm_path)) {
                    fprintf(stderr, "rcc: error: path contains unsafe characters for -S disassembly: %s\n",
                            path_is_shell_safe(tmp_obj_path) ? asm_path : tmp_obj_path);
                    return 1;
                }
                // Double quotes (not single quotes) so this works under both
                // sh and cmd.exe (wine/mingw); the temp file is removed via
                // remove() below instead of a shell "rm -f", which cmd.exe
                // doesn't understand.
                snprintf(cmd, sizeof(cmd), "%s -d -r --no-show-raw-insn \"%s\" > \"%s\"",
                         objdump, tmp_obj_path, asm_path);
                bool ok = system(cmd) == 0;
                // The data/rodata/bss dump is best-effort: PE objdump exits
                // non-zero when one of the -j sections is absent (e.g. a
                // .o with no data at all), which is a normal, not an error.
                snprintf(cmd, sizeof(cmd), "%s -s -j .text -j .data -j .rodata -j .bss \"%s\" >> \"%s\"",
                         objdump, tmp_obj_path, asm_path);
                if (system(cmd) != 0)
                    fprintf(stderr, "rcc: error: objdump failed for -S output\n");
                // Emit collected .ascii strings for kernel offsets
                for (CgAsciiStr *a = cg_ascii_strings; a; a = a->next) {
                    // codeql[cpp/path-injection,cpp/world-writable-file-creation]:
                    // appends to the same -S debug-output file opened above.
                    FILE *sf = fopen(asm_path, "a");
                    if (sf) {
                        fprintf(sf, "  .ascii \"%s\"\n", a->str);
                        fclose(sf);
                    }
                }
                cg_ascii_strings = NULL;
                remove(tmp_obj_path);
                if (!ok) {
                    fprintf(stderr, "rcc: error: objdump failed for -S output\n");
                    return 1;
                }
            }
        }

        if (!opt_S && !opt_dryrun) {
            OutPath *p = arena_alloc(sizeof(OutPath));
            p->path = asm_path;
            p->next = out_paths;
            out_paths = p;
        }
    }

    // Collected errors (GH #34): everything was diagnosed, now fail.
    if (error_count)
        return 1;

    // Assemble / Link if not just compiling to assembly or preprocessing
    if (!opt_S && !opt_E && !opt_deps_only) {
        if (opt_dryrun) {
            // Print what we would do
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "<built-in-assembler> -o %s", out_path);
            printf("%s\n", cmd);
            return 0;
        }

        out_paths = reverse(out_paths);

        if (opt_c) {
            // -c: codegen already produced binary .o files.
            // If -o was given, rename the output to the specified name.
            if (opt_o) {
                int status = 0;
                for (OutPath *p = out_paths; p; p = p->next) {
                    if (strcmp(p->path, out_path) != 0) {
                        if (rename(p->path, out_path) != 0) {
                            fprintf(stderr, "rcc: error: rename %s -> %s failed\n", p->path, out_path);
                            status = 1;
                        }
                    }
                }
                return status;
            }
            return 0;
        }
        // Linking: codegen already produced .o files; add them to linker command
        char *cmd = NULL;
        size_t cmd_len = 0, cmd_cap = 0;
        int status = 0;
        // For -o -, link to a temp file then stream it to stdout afterwards.
        char stdout_tmp[256] = "";
        const char *backend_out = out_path;
        if (opt_stdout) {
            snprintf(stdout_tmp, sizeof(stdout_tmp), "rcc_tmp_%d_stdout.out", _getpid());
            backend_out = stdout_tmp;
        }
#if defined(_WIN32) || defined(__MINGW32__)
        // Real gcc/mingw auto-appends ".exe" to an executable's -o name
        // when it lacks a recognized extension (every third-party
        // Makefile that does `$(CC) ... -o prog` expects `prog.exe` to
        // exist afterward). The external gcc.exe fallback below does
        // this itself via its own driver, but rcc_link()'s native PE
        // writer writes verbatim to whatever path it's given, so it has
        // to happen here to keep the native and external link paths
        // consistent with each other and with real gcc.
        char backend_out_exe[512];
        if (!opt_shared && !opt_stdout && !opt_relocatable) {
            size_t blen = strlen(backend_out);
            if (blen < 4 || strcmp(backend_out + blen - 4, ".exe") != 0) {
                snprintf(backend_out_exe, sizeof(backend_out_exe), "%s.exe", backend_out);
                backend_out = backend_out_exe;
            }
        }
#endif
        // Try the native linker first.
        // The native ELF/PE/Mach-O linker understands only -l/-L/-static
        // inputs (plus bare .a/.so positionals) and the -pie/-pic/-shared/
        // -static/-export-dynamic mode flags. It cannot honor -Wl, options
        // (rpath, soname, --start-group/--end-group, --as-needed,
        // --no-undefined, -v, -z, ...) or -nodefaultlibs; silently dropping
        // them would "link" with the wrong semantics (e.g. a shared lib
        // whose DT_RUNPATH/DT_SONAME never got written, or an archive
        // whose --start-group dependency closure was never resolved) — or,
        // for -Wl,-v, produce no output at all, breaking tools like muon
        // that probe the linker version through it. Detect these up front
        // and go straight to the external linker instead.
        bool native_link_capable = true;
        {
            const char *lp = libs;
            while (lp && *lp && native_link_capable) {
                while (*lp == ' ') lp++;
                if (!*lp) break;
                const char *end = lp;
                while (*end && *end != ' ') end++;
                size_t len = (size_t)(end - lp);
                bool wl = len >= 4 && !strncmp(lp, "-Wl,", 4);
                // -Wl,--out-implib,<path> is understood by the native PE
                // linker (see pe_write_out_implib(), hooked in below);
                // exempt it from the "can't honor arbitrary -Wl, options"
                // bailout so `-shared` + `--out-implib` still takes the
                // fast native path -- important since an external mingw
                // toolchain may not even be reachable at runtime (e.g.
                // rcc.exe running standalone under wine in CI).
                bool recognized_wl = wl && len > 4 + 13 && !strncmp(lp + 4, "--out-implib,", 13);
                if ((wl && !recognized_wl) ||
                    (len >= 14 && !strncmp(lp, "-nodefaultlibs", 14)) ||
                    (len == 9 && !strncmp(lp, "-nostdlib", 9)) ||
                    (len == 2 && !strncmp(lp, "-r", 2)))
                    native_link_capable = false;
                lp = end;
            }
        }
        // Decimal (_Decimal32/64/128) codegen emits __bid_*3/__bid_*2
        // runtime calls; those live in the bundled libdfp.a (lib/libdfp.a,
        // built from the vendored libbid core + rcc's wrapper layer). Link
        // it automatically, the same way -lm is added unconditionally:
        // locate it next to rcc's own include dir (RCC_INCDIR/../lib).
        // Only when this TU actually used a decimal type or literal, and
        // only when linking (-c/-S emit no calls and the archive is not
        // needed there).
        if (parser_used_decimal) {
#ifndef RCC_INCDIR
#define RCC_INCDIR "include"
#endif
            // Link the BUNDLED decimal runtime archive directly (positional
            // .a, which resolve_archives() always loads) rather than -ldfp:
            // a system libdfp.so found via the -l search uses GCC's
            // _Decimal64 ABI (XMM registers) for __bid_adddd3, incompatible
            // with rcc's plain bit-pattern ABI (GP registers) emitted for
            // the same symbol names. Our lib/libdfp.a is built for that ABI.
#ifdef _WIN32
            xappendf(&libs, &libs_len, &libs_cap, " %s/../lib/libdfp.lib", RCC_INCDIR);
#else
            xappendf(&libs, &libs_len, &libs_cap, " %s/../lib/libdfp.a", RCC_INCDIR);
#endif
        }
        if (native_link_capable) {
            int n_link_objs = 0;
            for (OutPath *p = out_paths; p; p = p->next) n_link_objs++;
            if (n_link_objs > 0) {
                char **link_objs = arena_alloc((size_t)n_link_objs * sizeof(char *));
                int i = 0;
                for (OutPath *p = out_paths; p; p = p->next)
                    link_objs[i++] = p->path;
                uint64_t t_link = opt_time ? now_us() : 0;
                if (getenv("RCC_LINK_DEBUG")) {
                    fprintf(stderr, "DBG link objs:");
                    for (int di = 0; di < n_link_objs; di++) fprintf(stderr, " %s", link_objs[di]);
                    fprintf(stderr, "\nDBG libs: %s\n", libs);
                }
                int native = rcc_link(backend_out, link_objs, n_link_objs,
                                      libs, opt_pie, opt_pic, opt_shared, opt_static,
                                      opt_export_dynamic);
                if (opt_time)
                    fprintf(stderr, "  link        %-20s: %6llu us\n", out_path,
                            (unsigned long long)(now_us() - t_link));
                if (native == 0) {
#if defined(_WIN32) || defined(__MINGW32__)
                    if (opt_shared && opt_out_implib) {
                        if (pe_write_out_implib(backend_out, opt_out_implib) != 0)
                            fprintf(stderr,
                                    "rcc: warning: -Wl,--out-implib,%s: %s has no exports, "
                                    "import library not written\n",
                                    opt_out_implib, backend_out);
                    }
#endif
                    if (opt_stdout) {
                        FILE *f = fopen(stdout_tmp, "rb");
                        if (f) {
                            char buf[65536];
                            size_t n;
                            while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
                                fwrite(buf, 1, n, stdout);
                            fclose(f);
                        }
                        remove(stdout_tmp);
                    }
                    for (OutPath *p = out_paths; p; p = p->next)
                        remove(p->path);
                    return 0;
                }
                // Native linker failed or unsupported; fall through to GCC.
                if (getenv("RCC_LINK_DEBUG")) {
                    fprintf(stderr, "rcc: LINK_DEBUG native linker returned %d for %s, falling back to %s\n",
                            native, out_path, GCC);
                }
            }
        }
        // Build the linker command line: backend compiler + output flag first
        //
        // CodeQL cpp/command-line-injection, cpp/uncontrolled-process-
        // operation: backend_out/each object path/the bundled runtime
        // object path below are embedded in a system() command string;
        // reject anything that could break out of the double quotes
        // wrapping each one (see path_is_shell_safe()'s doc comment for
        // why double-quote-and-reject beats escaping across both POSIX
        // sh and cmd.exe dialects) instead of leaving them unquoted --
        // an unquoted path containing a space (e.g. muon's own
        // "native/4 tryrun/..." build-test directory, or any Windows/
        // macOS path with a space in it) previously split into extra
        // shell words: `ld` then reported "cannot find <tail>: No such
        // file or directory" instead of ever seeing the real, intended
        // single path.
        if (!path_is_shell_safe(backend_out)) {
            fprintf(stderr, "rcc: error: output path contains unsafe characters for linking: %s\n",
                    backend_out);
            free(libs);
            return 1;
        }
        for (OutPath *p = out_paths; p; p = p->next) {
            if (!path_is_shell_safe(p->path)) {
                fprintf(stderr, "rcc: error: object path contains unsafe characters for linking: %s\n",
                        p->path);
                free(libs);
                return 1;
            }
        }
#ifdef __APPLE__
        if (opt_relocatable) {
            // `-r` produces a relocatable object, not an executable or
            // shared library -- the flags below are meaningless (or
            // actively rejected by ld64) in that mode: `-arch`/`-isysroot`
            // only matter for resolving/linking against real system
            // frameworks, and `-Wl,-undefined,dynamic_lookup` (defer
            // undefined-symbol resolution to runtime dyld) makes no sense
            // for an object that isn't being dynamically linked at all.
            xappendf(&cmd, &cmd_len, &cmd_cap, GCC " -r -o \"%s\"", backend_out);
        } else
            xappendf(&cmd, &cmd_len, &cmd_cap,
                     GCC " -o \"%s\" -arch arm64"
                         " -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
                         " -Wl,-undefined,dynamic_lookup",
                     backend_out);
#else
        if (opt_pie)
            xappendf(&cmd, &cmd_len, &cmd_cap, GCC " -pie -o \"%s\"", backend_out);
        else if (opt_pic)
            xappendf(&cmd, &cmd_len, &cmd_cap, GCC " -o \"%s\"", backend_out);
        else
            xappendf(&cmd, &cmd_len, &cmd_cap, GCC " -no-pie -o \"%s\"", backend_out);
#endif

        // Codegen already produced .o files; add them directly to linker command
        for (OutPath *p = out_paths; p; p = p->next)
            xappendf(&cmd, &cmd_len, &cmd_cap, " \"%s\"", p->path);

#if defined(_WIN32) || defined(__MINGW32__)
        if (!opt_relocatable) {
            struct stat libst;
#ifdef RCC_INCDIR
            const char *rcc_lib = RCC_INCDIR "/../lib/rcc_mingw.obj";
            if (stat("lib/rcc_mingw.obj", &libst) != 0 && stat(rcc_lib, &libst) == 0) {
                if (!path_is_shell_safe(rcc_lib)) {
                    fprintf(stderr, "rcc: error: runtime object path contains unsafe characters for linking: %s\n",
                            rcc_lib);
                    free(libs);
                    return 1;
                }
                xappendf(&cmd, &cmd_len, &cmd_cap, " \"%s\"", rcc_lib);
            } else
#endif
                if (stat("lib/rcc_mingw.obj", &libst) == 0)
                xappendf(&cmd, &cmd_len, &cmd_cap, " lib/rcc_mingw.obj");
        }
#endif
#ifdef __APPLE__
        {
            struct stat libst;
            // Try absolute path first (RCC_INCDIR/../lib/darwin.o)
#ifdef RCC_INCDIR
            const char *rcc_darwin = RCC_INCDIR "/../lib/rcc_darwin.dylib";
            if (stat(rcc_darwin, &libst) == 0) {
                if (!path_is_shell_safe(rcc_darwin)) {
                    fprintf(stderr, "rcc: error: runtime object path contains unsafe characters for linking: %s\n",
                            rcc_darwin);
                    free(libs);
                    return 1;
                }
                xappendf(&cmd, &cmd_len, &cmd_cap, " \"%s\"", rcc_darwin);
            } else
#endif
                if (stat("lib/rcc_darwin.dylib", &libst) == 0)
                xappendf(&cmd, &cmd_len, &cmd_cap, " lib/rcc_darwin.dylib");
        }
#endif

        if (libs_len)
            xappendf(&cmd, &cmd_len, &cmd_cap, "%s", libs);

        // RCC's own codegen emits a genuine external call for math.h
        // functions (fabs/sqrt/pow/...) rather than inlining the simple
        // ones to native FP instructions the way GCC/Clang do, so a
        // fallback link needs libm even for programs whose *user*
        // command line never mentioned -lm (an equivalent GCC-compiled
        // build wouldn't reference libm at all, since e.g. its fabs()
        // call never survives past codegen). Matches the native ELF
        // linker's own unconditional libm.so.6 DT_NEEDED addition.
        // Only add it when the user didn't already pass -lm: a duplicate
        // makes recent macOS ld warn "ignoring duplicate libraries:
        // '-lm'", which pollutes callers that capture the link output.
        // `-r` (partial/relocatable link, e.g. busybox's own two-stage
        // `applets/built-in.o` build) must never get an auto-added -lm:
        // `-r` disables shared linking, so `ld` demands a *static*
        // libm.a -- which this system may not even have installed
        // (verified directly against real gcc: `gcc -r -lm a.o b.o`
        // fails "cannot find -lm" here even though `-lm` alone links
        // fine against the shared libm.so in every other mode).
        bool have_lm = false;
        for (const char *p = libs; p && (p = strstr(p, "-lm")); p += 3) {
            bool start_ok = (p == libs) || p[-1] == ' ';
            bool end_ok = (p[3] == '\0' || p[3] == ' ');
            if (start_ok && end_ok) {
                have_lm = true;
                break;
            }
        }
        if (!have_lm && !opt_relocatable)
            xappendf(&cmd, &cmd_len, &cmd_cap, " -lm");

        if (opt_dryrun) {
            puts(cmd);
            free(libs);
            free(cmd);
            return 0;
        }
        if (!status) {
            uint64_t t_link = opt_time ? now_us() : 0;
            status = system(cmd);
            if (opt_time)
                fprintf(stderr, "  link        %-20s: %6llu us\n", out_path,
                        (unsigned long long)(now_us() - t_link));
            if (status != 0)
                fprintf(stderr, "rcc: error: linker %s failed with code %d\n", cmd, status);
        }

        // For -o -, stream the linked backend output to stdout.
        if (opt_stdout && status == 0) {
            FILE *f = fopen(stdout_tmp, "rb");
            if (f) {
                char buf[65536];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
                    fwrite(buf, 1, n, stdout);
                fclose(f);
            }
        }
        if (opt_stdout)
            remove(stdout_tmp);

        // Cleanup temp files
        if (getenv("RCC_KEEP_TMP")) {
            for (OutPath *p = out_paths; p; p = p->next)
                fprintf(stderr, "DBG keep tmp: %s\n", p->path);
        }
        for (OutPath *p = out_paths; p; p = p->next)
            remove(p->path);
        free(libs);
        free(cmd);

        return status ? 1 : 0;
    }
    if (opt_E && pp_out != stdout) fclose(pp_out);
    return 0;
}
