// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from chibicc by Rui Ueyama.
#include "rcc.h"
#include "asm.h"
#include "link.h"
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#define _getpid getpid
#endif
#include <sys/stat.h>
#include <time.h>

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
static OutPath *obj_paths;


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

void help(void) {
    printf("rcc %s %s - Copyright 2026 Hosokawa-t and Reini Urban\n", VERSION, MACHINE);
    printf("Licensed under the GNU Lesser General Public License v2.1 or later\n");
    printf("rcc [options...] [-o outfile] [-c] infile(s)...\n");
    printf("Options:\n"
           "-I path             add include path\n"
           "-Dname[=val]        define a macro\n"
           "-Uname              undefine a macro\n"
           "-E                  preprocessor-only\n"
           "-S                  assemble-only\n"
           "-c                  compile-only\n"
           "-o file             set output filename\n"
           "-O0                 disable peephole optimizer\n"
           "-O1                 enable peephole + CTFE optimizations\n"
           "-g                  emit DWARF line-number debug info\n"
           "-W                  enable more compiler warnings\n"
           "-Werror             treat all warnings as errors\n"
           "-Wno-homoglyph      disable Unicode indentifer homoglyph warnings\n"
           "-Lpath              add linker path\n"
           "-lname              add lib\n"
           "-pthread            link with pthreads library\n"
           "-shared             create shared library\n"
           "-static             link statically\n"
           "-Wl,<opt>           pass option to linker\n"
           "-mms-bitfields      use MSVC bitfield layout by default\n"
           "-mno-ms-bitfields   use GCC bitfield layout by default\n"
           "-pie|-fPIE|-fpie    generate position-independent executable\n"
           "-fPIC|-fpic         generate position-independent code\n"
           "-time               print timing for each compilation substep\n"
           "-v                  be more verbose\n"
           "-###                dry-run (print commands, don't execute)\n"
           "-dM                 dump all macro definitions (use with -E)\n"
           "-fdump-ast          dump AST for debugging\n"
           "-print-search-dirs  print install, include and library paths\n"
           "-dumpmachine        print target version\n"
           "-dumpversion        print gcc compatibility version\n"
           "--help\n"
           "--version\n");
}

bool opt_O0 = false;
bool opt_O1 = false;
bool opt_W = false;
bool opt_Werror = false;
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
    bool opt_S = false;
    bool opt_c = false;
    bool opt_E = false;
    bool opt_o = false;
    bool opt_stdout = false; // -o - : write final output to stdout
    char libs[512] =
#ifdef _WIN32
        " -lm"
#else
        ""
#endif
        ;
    int libs_len = strlen(libs);

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
        } else if (!strcmp(argv[i], "-O1") || !strcmp(argv[i], "-O2") || !strcmp(argv[i], "-O3")) {
            opt_O1 = true;
        } else if (!strcmp(argv[i], "-W")) {
            opt_W = true;
        } else if (!strcmp(argv[i], "-Werror")) {
            opt_Werror = true;
        } else if (!strcmp(argv[i], "-Wno-homoglyph")) {
            opt_Wno_homoglyph = true;
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
            int n = snprintf(libs + libs_len, sizeof(libs) - libs_len, " %s", argv[i]);
            if (n > 0 && libs_len + n < (int)sizeof(libs))
                libs_len += n;
        } else if (!strcmp(argv[i], "--as-needed") ||
                   !strcmp(argv[i], "--no-as-needed")) {
            int n = snprintf(libs + libs_len, sizeof(libs) - libs_len, " -Wl,%s", argv[i]);
            if (n > 0 && libs_len + n < (int)sizeof(libs))
                libs_len += n;
        } else if (!strcmp(argv[i], "-z")) {
            if (++i >= argc) {
                fprintf(stderr, "error: missing argument for -z\n");
                return 1;
            }
            int n = snprintf(libs + libs_len, sizeof(libs) - libs_len, " -Wl,-z,%s", argv[i]);
            if (n > 0 && libs_len + n < (int)sizeof(libs))
                libs_len += n;
        } else if (!strcmp(argv[i], "-rpath")) {
            if (++i >= argc) {
                fprintf(stderr, "error: missing argument for -rpath\n");
                return 1;
            }
            int n = snprintf(libs + libs_len, sizeof(libs) - libs_len,
                             " -Wl,-rpath,%s", argv[i]);
            if (n > 0 && libs_len + n < (int)sizeof(libs))
                libs_len += n;
        } else if (!strncmp(argv[i], "-l", 2) || !strncmp(argv[i], "-L", 2) ||
                   !strcmp(argv[i], "-shared") || !strcmp(argv[i], "-static") ||
                   !strncmp(argv[i], "-Wl,", 4)) {
            int n = snprintf(libs + libs_len, sizeof(libs) - libs_len, " %s", argv[i]);
            if (n > 0 && libs_len + n < (int)sizeof(libs))
                libs_len += n;
        } else if (!strcmp(argv[i], "-soname")) {
            if (++i >= argc) {
                fprintf(stderr, "error: missing argument for -soname\n");
                return 1;
            }
            int n = snprintf(libs + libs_len, sizeof(libs) - libs_len,
                             " -Wl,-soname,%s", argv[i]);
            if (n > 0 && libs_len + n < (int)sizeof(libs))
                libs_len += n;
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
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "rcc: warning: ignored unknown option %s\n", argv[i]);
        } else {
            if (n_inputs < 64)
                input_files[n_inputs++] = argv[i];
        }
    }

    if (n_inputs == 0) {
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
            asm_path = opt_o ? out_path : format("%s.o", path_basename(cur_path));
        } else if (opt_c) {
            asm_path = opt_o ? out_path : format("%s.o", path_basename(cur_path));
        } else {
            asm_path = format("rcc_tmp_%d_%d_%s.o", _getpid(), fi, path_basename(cur_path));
        }

        // Tokenize and Parse
        char *contents = read_file(cur_path);
        str_intern_resize(strlen(contents)); // size hash for this file

        // Always preprocess - opt_E just outputs preprocessed result
        uint64_t t0 = opt_time ? now_us() : 0;
        char *preprocessed = preprocess(cur_path, contents);
        if (opt_time)
            fprintf(stderr, "  preprocess  %s: %6llu us\n", cur_path,
                    (unsigned long long)(now_us() - t0));

        if (opt_E) {
            printf("%s", preprocessed);
            continue;
        }

        t0 = opt_time ? now_us() : 0;
        Token *tok = tokenize(cur_path, preprocessed);
        if (opt_time)
            fprintf(stderr, "  lex         %s: %6llu us\n", cur_path,
                    (unsigned long long)(now_us() - t0));

        t0 = opt_time ? now_us() : 0;
        Program *prog = parse(tok);
        prog->in_path = cur_path;
        if (opt_time)
            fprintf(stderr, "  parse       %s: %6llu us\n", cur_path,
                    (unsigned long long)(now_us() - t0));

        if (opt_fdump_ast)
            dump_ast(prog);

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
        if (opt_O1) {
            t0 = opt_time ? now_us() : 0;
            optimize(prog);
            if (opt_time)
                fprintf(stderr, "  opt(CTFE)   %s: %6llu us\n", cur_path,
                        (unsigned long long)(now_us() - t0));
        }

        if (!opt_dryrun) {
            time_peep_us = 0;
            t0 = opt_time ? now_us() : 0;
            struct ObjFile *obj = codegen(prog);
            if (opt_time) {
                uint64_t cg_total = now_us() - t0;
                fprintf(stderr, "  codegen     %s: %6llu us\n", cur_path,
                        (unsigned long long)(cg_total - time_peep_us));
                if (!opt_O0)
                    fprintf(stderr, "  peephole    %s: %6llu us\n", cur_path,
                            (unsigned long long)time_peep_us);
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
                snprintf(cmd, sizeof(cmd), "%s -s -j .data -j .rodata -j .bss \"%s\" >> \"%s\"",
                         objdump, tmp_obj_path, asm_path);
                if (system(cmd) != 0)
                    fprintf(stderr, "rcc: error: objdump failed for -S output\n");
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
            // -c: codegen already produced binary .o files; rename to out_path
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

        // Linking: codegen already produced .o files; add them to linker command
        char cmd[4096] = "";
        int status = 0;
        // For -o -, link to a temp file then stream it to stdout afterwards.
        char stdout_tmp[256] = "";
        const char *backend_out = out_path;
        if (opt_stdout) {
            snprintf(stdout_tmp, sizeof(stdout_tmp), "rcc_tmp_%d_stdout.out", _getpid());
            backend_out = stdout_tmp;
        }
        // Try the native linker first.
        {
            int n_link_objs = 0;
            for (OutPath *p = out_paths; p; p = p->next) n_link_objs++;
            if (n_link_objs > 0) {
                char **link_objs = arena_alloc((size_t)n_link_objs * sizeof(char *));
                int i = 0;
                for (OutPath *p = out_paths; p; p = p->next)
                    link_objs[i++] = p->path;
                uint64_t t_link = opt_time ? now_us() : 0;
                int native = rcc_link(backend_out, link_objs, n_link_objs,
                                      libs, opt_pie, opt_pic, false);
                if (opt_time)
                    fprintf(stderr, "  native link %s: %6lu us\n", out_path,
                            (unsigned long)(now_us() - t_link));
                if (native == 0) {
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
            }
        }


        if (obj_paths) {
            obj_paths = reverse(obj_paths);
            for (OutPath *p = obj_paths; p; p = p->next) {
                strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
                strncat(cmd, p->path, sizeof(cmd) - strlen(cmd) - 1);
            }
        }

#ifdef __APPLE__
        snprintf(cmd, sizeof(cmd), GCC " -o %s -arch arm64"
                                       " -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
                                       " -Wl,-undefined,dynamic_lookup",
                 backend_out);
#else
        if (opt_pie)
            snprintf(cmd, sizeof(cmd), GCC " -pie -o %s", backend_out);
        else if (opt_pic)
            snprintf(cmd, sizeof(cmd), GCC " -o %s", backend_out);
        else
            snprintf(cmd, sizeof(cmd), GCC " -no-pie -o %s", backend_out);
#endif

        // Codegen already produced .o files; add them directly to linker command
        for (OutPath *p = out_paths; p; p = p->next) {
            strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
            strncat(cmd, p->path, sizeof(cmd) - strlen(cmd) - 1);
        }

#if defined(_WIN32) || defined(__MINGW32__)
        {
            struct stat libst;
#ifdef RCC_INCDIR
            const char *rcc_lib = RCC_INCDIR "/../lib/rcc_mingw.obj";
            if (stat("lib/rcc_mingw.obj", &libst) != 0 && stat(rcc_lib, &libst) == 0)
                snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), " %s", rcc_lib);
            else
#endif
                if (stat("lib/rcc_mingw.obj", &libst) == 0)
                snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), " lib/rcc_mingw.obj");
        }
#endif
#ifdef __APPLE__
        {
            struct stat libst;
            // Try absolute path first (RCC_INCDIR/../lib/darwin.o)
#ifdef RCC_INCDIR
            const char *rcc_darwin = RCC_INCDIR "/../lib/rcc_darwin.dylib";
            if (stat(rcc_darwin, &libst) == 0)
                snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), " %s", rcc_darwin);
            else
#endif
                if (stat("lib/rcc_darwin.dylib", &libst) == 0)
                snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), " lib/rcc_darwin.dylib");
        }
#endif

        if (libs_len)
            strncat(cmd, libs, sizeof(cmd) - strlen(cmd) - 1);

        if (opt_dryrun) {
            puts(cmd);
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

        return status ? 1 : 0;
    }
    return 0;
}
