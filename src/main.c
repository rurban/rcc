// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from chibicc by Rui Ueyama.
#include "rcc.h"
#include "asm.h"
#include "codegen_asm.h"
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

    int filemax = 10 * 1024 * 1024;
    char *buf = arena_alloc(filemax);
    size_t size = fread(buf, 1, filemax - 2, fp);
    if (!feof(fp)) {
        error("%s: file too large", path);
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


// Replace the extension of filename. Strips .c/.i/.s and appends new_ext.
static char *replace_ext(char *filename, char *new_ext) {
    char *dot = strrchr(filename, '.');
    if (dot && (strcmp(dot, ".c") == 0 || strcmp(dot, ".i") == 0 || strcmp(dot, ".s") == 0))
        return format("%.*s%s", (int)(dot - filename), filename, new_ext);
    return format("%s%s", filename, new_ext);
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
           "-Wp,-MMD,file       write Make dependency rules\n"
           "-fmacro-prefix-map=old=new  remap paths in diagnostics\n"
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
bool opt_W = false;
bool opt_Werror = false;
bool opt_pedantic = false;
bool opt_Werror_unknown = false;
bool opt_Wno_homoglyph = false;
bool opt_dryrun = false;
bool opt_dM = false;
bool opt_fdump_ast = false;
bool opt_g = false;
bool opt_pie = false;
bool opt_pic = false;
bool opt_time = false;
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
// -Wp,-MMD,<file>: write Make dependency rules
const char *opt_depfile = NULL;
// -fmacro-prefix-map=old=new
const char *opt_prefix_map_old = NULL;
const char *opt_prefix_map_new = NULL;

bool sse42_available = false;

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
    char *input_files[64];
    int n_inputs = 0;
    // -include <file>: pre-include files before main source
    const char *preinclude_files[64];
    int nb_preinclude_files = 0;
    bool opt_S = false;
    bool opt_c = false;
    bool opt_E = false;
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
            opt_O0 = true;
        } else if (!strcmp(argv[i], "-O1")) {
            opt_O1 = true;
        } else if (!strcmp(argv[i], "-O2") || !strcmp(argv[i], "-O3")) {
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
        } else if (!strcmp(argv[i], "-W")) {
            opt_W = true;
        } else if (!strcmp(argv[i], "-Werror")) {
            opt_Werror = true;
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
                   !strcmp(argv[i], "-g2") || !strcmp(argv[i], "-g3")) {
            opt_g = true;
        } else if (!strcmp(argv[i], "-g0")) {
            opt_g = false;
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
        } else if (!strcmp(argv[i], "-o")) {
            if (++i >= argc) {
                fprintf(stderr, "error: missing argument for -o\n");
                return 1;
            }
            out_path = argv[i];
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
        } else if (!strncmp(argv[i], "-l", 2) || !strncmp(argv[i], "-L", 2) ||
                   !strcmp(argv[i], "-shared") || !strcmp(argv[i], "-static") ||
                   !strcmp(argv[i], "-nodefaultlibs") ||
                   !strncmp(argv[i], "-Wl,", 4)) {
            // -nodefaultlibs: link-stage-only flag (unlike -nostdlib, still
            // links the standard startup files) — forward it to the
            // backend linker invocation, which already understands it.
            xappendf(&libs, &libs_len, &libs_cap, " %s", argv[i]);
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
        } else if (!strcmp(argv[i], "-xc") || (!strcmp(argv[i], "-x") && i + 1 < argc && !strcmp(argv[i + 1], "c"))) {
            if (!strcmp(argv[i], "-x")) i++; // skip "c"
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
        } else if (!strncmp(argv[i], "-fexec-charset=", 15)) {
            opt_exec_charset = argv[i] + 15;
        } else if (!strncmp(argv[i], "-Wno-", 5) ||
                   !strncmp(argv[i], "-Werror=", 8)) {
            ; // silently ignored
        } else if (!strcmp(argv[i], "-pedantic-errors")) {
            opt_Werror = true;
            opt_pedantic = true;
        } else if (!strcmp(argv[i], "-pedantic") || !strcmp(argv[i], "-Wpedantic")) {
            opt_pedantic = true;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            if (opt_Werror_unknown) {
                fprintf(stderr, "rcc: error: unrecognized command-line option '%s'\n", argv[i]);
                return 1;
            }
            fprintf(stderr, "rcc: warning: ignored unknown option %s\n", argv[i]);
        } else {
            // Object files and libraries go directly to the linker, in
            // argv order (interleaved with -Wl flags). They are caller
            // files: never delete them like our own temp objects.
            const char *ext = strrchr(argv[i], '.');
            if (ext && (!strcmp(ext, ".o") || !strcmp(ext, ".lo") || !strcmp(ext, ".a") || !strcmp(ext, ".so")
#ifdef _WIN32
                        || !strcmp(ext, ".obj") || !strcmp(ext, ".dll") || !strcmp(ext, ".lib")
#elif defined(__APPLE__)
                        || !strcmp(ext, ".dylib")
#endif
                            )) {
                xappendf(&libs, &libs_len, &libs_cap, " %s", argv[i]);
                have_link_inputs = true;
            } else if (n_inputs < 64) {
                input_files[n_inputs++] = argv[i];
            }
        }
    }

    // Allow link-only mode: object files from command line go to the linker
    if (n_inputs == 0 && !have_link_inputs) {
        fprintf(stderr, "rcc: fatal error: no input files\n");
        return 1;
    }

    // -c -o - is impossible: object files require random access (seek).
    if (opt_c && opt_stdout) {
        fprintf(stderr, "rcc: error: -c -o - is not supported (object files require seekable output)\n");
        return 1;
    }

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
        Token *tok = preprocess(cur_path, contents);
        if (opt_time)
            fprintf(stderr, "  preprocess  %s: %6llu us\n", cur_path,
                    (unsigned long long)(now_us() - t0));
        // Write Make dependency file (-Wp,-MMD,<file>)
        write_dep_file(out_path, cur_path);

        if (opt_dM) {
            printf("%s", dump_macros_text());
            continue;
        }

        if (opt_E) {
            pp_print_tokens(tok);
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
            char *dot = strrchr(cur_path, '.');
            bool is_dot_cap_s = dot && strcmp(dot, ".S") == 0;
            bool is_dot_s = dot && strcmp(dot, ".s") == 0;
            if (is_dot_cap_s || is_dot_s) {
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
            fprintf(stderr, "  parse       %s: %6llu us\n", cur_path,
                    (unsigned long long)(now_us() - t0));

        if (opt_fdump_ast)
            dump_ast(prog);

        // Parse errors were collected (GH #34): the AST is incomplete, so
        // skip typecheck/codegen for this file; more inputs may still be
        // parsed for their diagnostics. Failure exit happens after the loop.
        if (error_count)
            continue;

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
            fprintf(stderr, "  typecheck   %s: %6llu us\n", cur_path,
                    (unsigned long long)(now_us() - t0));

        // CTFE runs only with -O1; peephole skipped with -O0.
        if (opt_O1 || opt_finline || opt_funroll) {
            t0 = opt_time ? now_us() : 0;
            optimize(prog);
            if (opt_time)
                fprintf(stderr, "  opt         %s: %6llu us\n", cur_path,
                        (unsigned long long)(now_us() - t0));
        }

        if (!opt_dryrun) {
            t0 = opt_time ? now_us() : 0;
            struct ObjFile *obj = codegen(prog);
            if (opt_time) {
                fprintf(stderr, "  codegen     %s: %6llu us\n", cur_path,
                        (unsigned long long)(now_us() - t0));
            }
            // Write binary .o file
            char *tmp_obj_path = asm_path;
            if (opt_S) {
                tmp_obj_path = format("%s.tmp.o", asm_path);
            }
            int wr;
#ifdef _WIN32
            wr = coff_write(obj, tmp_obj_path);
#elif __APPLE__
            wr = macho_write(obj, tmp_obj_path);
#else
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
    if (!opt_S && !opt_E) {
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
        // Build the linker command line: backend compiler + output flag first
#ifdef __APPLE__
        xappendf(&cmd, &cmd_len, &cmd_cap,
                 GCC " -o %s -arch arm64"
                     " -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
                     " -Wl,-undefined,dynamic_lookup",
                 backend_out);
#else
        if (opt_pie)
            xappendf(&cmd, &cmd_len, &cmd_cap, GCC " -pie -o %s", backend_out);
        else if (opt_pic)
            xappendf(&cmd, &cmd_len, &cmd_cap, GCC " -o %s", backend_out);
        else
            xappendf(&cmd, &cmd_len, &cmd_cap, GCC " -no-pie -o %s", backend_out);
#endif

        // Codegen already produced .o files; add them directly to linker command
        for (OutPath *p = out_paths; p; p = p->next)
            xappendf(&cmd, &cmd_len, &cmd_cap, " %s", p->path);

#if defined(_WIN32) || defined(__MINGW32__)
        {
            struct stat libst;
#ifdef RCC_INCDIR
            const char *rcc_lib = RCC_INCDIR "/../lib/rcc_mingw.obj";
            if (stat("lib/rcc_mingw.obj", &libst) != 0 && stat(rcc_lib, &libst) == 0)
                xappendf(&cmd, &cmd_len, &cmd_cap, " %s", rcc_lib);
            else
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
            if (stat(rcc_darwin, &libst) == 0)
                xappendf(&cmd, &cmd_len, &cmd_cap, " %s", rcc_darwin);
            else
#endif
                if (stat("lib/rcc_darwin.dylib", &libst) == 0)
                xappendf(&cmd, &cmd_len, &cmd_cap, " lib/rcc_darwin.dylib");
        }
#endif

        if (libs_len)
            xappendf(&cmd, &cmd_len, &cmd_cap, "%s", libs);

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
                fprintf(stderr, "  link        %s: %6lu us\n", out_path,
                        (unsigned long)(now_us() - t_link));
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
        for (OutPath *p = out_paths; p; p = p->next)
            remove(p->path);
        free(libs);
        free(cmd);

        return status ? 1 : 0;
    }
    return 0;
}
