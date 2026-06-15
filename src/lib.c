// SPDX-License-Identifier: LGPL-2.1-or-later
// rcc library — compile to shared library, dlopen/dlsym for in-process use.
// Uses the same pipeline as main.c: preprocess → tokenize → parse →
// typecheck → optimize → codegen → assemble+link (system gcc).
#include "rcc.h"
#include "rcc_lib.h"
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#ifndef GCC
#ifdef _WIN32
#define GCC "gcc.exe"
#else
#define GCC "gcc"
#endif
#endif

static bool lib_initialized = false;

// clone of main.c's read_file (static there, needed here)
static char *read_file(char *path) {
    bool is_stdin = strcmp(path, "-") == 0;
    FILE *fp = is_stdin ? stdin : fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "rcc_lib: cannot open %s\n", path);
        return NULL;
    }
    int filemax = 10 * 1024 * 1024;
    char *buf = arena_alloc(filemax);
    size_t size = fread(buf, 1, filemax - 2, fp);
    if (!feof(fp))
        fprintf(stderr, "rcc_lib: %s: file too large\n", path);
    if (size == 0 || buf[size - 1] != '\n')
        buf[size++] = '\n';
    if (!is_stdin)
        fclose(fp);
    buf[size] = '\0';
    return buf;
}

struct RCCLib {
    char *out_path; // compiled shared library path
    char *asm_path; // assembly temp file path
    void *dlhandle; // dlopen/LoadLibrary handle
    bool compiled; // compile succeeded
};

static char *build_tmp_path(const char *basename, const char *suffix) {
    const char *tmpdir;
    char tmpbuf[512];
#ifdef _WIN32
    if (!GetTempPathA(sizeof(tmpbuf), tmpbuf))
        strcpy(tmpbuf, ".");
    tmpdir = tmpbuf;
#else
    tmpdir = "/tmp";
#endif
    size_t len = strlen(tmpdir) + 1 + strlen(basename) + strlen(suffix) + 32;
    char *path = malloc(len);
    snprintf(path, len, "%s/rcc_lib_%s%s", tmpdir, basename, suffix);
    return path;
}

static bool file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static char *path_basename_noext(const char *path) {
    const char *base = strrchr(path, '/');
    if (base) base++;
    else
        base = path;
#ifdef _WIN32
    const char *bs = strrchr(path, '\\');
    if (bs && bs > base) base = bs + 1;
#endif
    char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);
    char *name = malloc(len + 1);
    memcpy(name, base, len);
    name[len] = '\0';
    return name;
}

// Run the assembler+linker to produce a shared library from assembly.
static int assemble_and_link(const char *asm_path, const char *out_path,
                             const char *extra_link_flags) {
    char cmd[4096];
#ifdef __APPLE__
    snprintf(cmd, sizeof(cmd),
             "cc -shared -o %s -arch arm64 "
             "-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk "
             "-Wl,-undefined,dynamic_lookup %s",
             out_path, asm_path);
#elif defined(_WIN32)
    snprintf(cmd, sizeof(cmd),
             GCC " -shared -o %s %s -Wl,--export-all-symbols",
             out_path, asm_path);
#else
    snprintf(cmd, sizeof(cmd),
             GCC " -shared -fPIC -o %s %s", out_path, asm_path);
#endif
#ifdef _WIN32
    {
        struct stat libst;
        if (stat("lib/rcc_mingw.obj", &libst) == 0) {
            strncat(cmd, " lib/rcc_mingw.obj", sizeof(cmd) - strlen(cmd) - 1);
#ifdef RCC_INCDIR
        } else {
            const char *rcc_lib = RCC_INCDIR "/../lib/rcc_mingw.obj";
            if (stat(rcc_lib, &libst) == 0) {
                strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
                strncat(cmd, rcc_lib, sizeof(cmd) - strlen(cmd) - 1);
            }
#endif
        }
    }
#endif
    if (extra_link_flags && extra_link_flags[0]) {
        strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
        strncat(cmd, extra_link_flags, sizeof(cmd) - strlen(cmd) - 1);
    }
    return system(cmd);
}

// ── public API ─────────────────────────────────────────────────────

RCCLib *rcc_lib_new(void) {
    if (!lib_initialized) {
        init_keywords();
        init_builtins();
        init_builtin_names();
        lib_initialized = true;
    }
    RCCLib *lib = calloc(1, sizeof(RCCLib));
    return lib;
}

int rcc_lib_compile_string(RCCLib *lib, const char *src) {
    char *tmp_src = build_tmp_path("src", ".c");
    FILE *f = fopen(tmp_src, "w");
    if (!f) {
        free(tmp_src);
        return -1;
    }
    fputs(src, f);
    if (src[strlen(src) - 1] != '\n') fputc('\n', f);
    fclose(f);
    int ret = rcc_lib_compile_file(lib, tmp_src);
    unlink(tmp_src);
    free(tmp_src);
    return ret;
}

int rcc_lib_compile_file(RCCLib *lib, const char *path) {
    return rcc_lib_compile_file_ex(lib, path, NULL, NULL);
}

// Reset compiler global state (command-line macros, -I paths, bitfield
// layout option, ...) to a fresh-process baseline.  Called before every
// compile so flags from a previous rcc_lib_compile_file* call don't leak
// into the next one.
void rcc_lib_reset(RCCLib *lib) {
    (void)lib;
    rcc_reset_state();
    opt_ms_bitfields =
#ifdef _WIN32
        true;
#else
        false;
#endif
}

// Apply a whitespace-separated list of compile flags: -Dname[=val],
// -Uname, -Ipath (no space between flag and argument), -mms-bitfields,
// -mno-ms-bitfields.  Unrecognized flags are ignored.
static void apply_cflags(const char *cflags) {
    if (!cflags || !cflags[0]) return;
    char *copy = strdup(cflags);
    char *sv = NULL;
    for (char *tok = strtok_r(copy, " \t", &sv); tok; tok = strtok_r(NULL, " \t", &sv)) {
        if (!strncmp(tok, "-D", 2) && tok[2])
            add_define(tok + 2);
        else if (!strncmp(tok, "-U", 2) && tok[2])
            add_undef(tok + 2);
        else if (!strncmp(tok, "-I", 2) && tok[2])
            add_include_path(tok + 2);
        else if (!strcmp(tok, "-mms-bitfields"))
            opt_ms_bitfields = true;
        else if (!strcmp(tok, "-mno-ms-bitfields"))
            opt_ms_bitfields = false;
        /* else: ignore unknown flags */
    }
    free(copy);
}

int rcc_lib_compile_file_ex(RCCLib *lib, const char *path,
                            const char *include_dir,
                            const char *extra_link_flags) {
    return rcc_lib_compile_file_ex2(lib, path, include_dir, NULL, extra_link_flags);
}

int rcc_lib_compile_file_ex2(RCCLib *lib, const char *path,
                             const char *include_dir, const char *cflags,
                             const char *extra_link_flags) {
    rcc_lib_reset(lib);
    if (!file_exists(path)) return -1;
    if (include_dir && include_dir[0])
        add_include_path(include_dir);
    apply_cflags(cflags);

    char *base = path_basename_noext(path);
    lib->asm_path = build_tmp_path(base, ".s");
    lib->out_path = build_tmp_path(base,
#ifdef _WIN32
                                   ".dll"
#elif defined(__APPLE__)
                                   ".dylib"
#else
                                   ".so"
#endif
    );
    free(base);

    // ── compilation pipeline ───────────────────────────────────────

    char *contents = read_file((char *)path);
    if (!contents) return -1;

    char *preprocessed = preprocess((char *)path, contents);
    Token *tok = tokenize((char *)path, preprocessed);
    Program *prog = parse(tok);
    prog->in_path = (char *)path;

    // Type system / semantic checks
    for (TLItem *item = prog->items; item; item = item->next) {
        if (item->kind != TL_FUNC) continue;
        Node *n = item->fn->body;
        while (n) {
            check_type(n);
            n = n->next;
        }
    }

    // CTFE optimization (default: on)
    optimize(prog);

    // Redirect stdout to assembly file (restored by caller)
    if (!freopen(lib->asm_path, "w", stdout)) {
        fprintf(stderr, "rcc_lib: cannot open %s\n", lib->asm_path);
        return -1;
    }

    codegen(prog);
    fclose(stdout);


    // Assemble + link to shared library
    int ret = assemble_and_link(lib->asm_path, lib->out_path, extra_link_flags);
    if (ret != 0) {
        fprintf(stderr, "rcc_lib: link failed for %s\n", path);
        return ret;
    }

    lib->compiled = true;
    return 0;
}

void *rcc_lib_get_symbol(RCCLib *lib, const char *name) {
    if (!lib->compiled || !lib->out_path) return NULL;

    if (!lib->dlhandle) {
#ifdef _WIN32
        lib->dlhandle = LoadLibraryA(lib->out_path);
        if (!lib->dlhandle) {
            fprintf(stderr, "rcc_lib: LoadLibrary %s failed\n", lib->out_path);
            return NULL;
        }
        return (void *)GetProcAddress((HMODULE)lib->dlhandle, name);
#else
        /* RTLD_NOW (not RTLD_LAZY): resolve all symbols at dlopen time so an
         * undefined symbol (e.g. a multi-file TCC test whose companion
         * "+"-file wasn't linked in) makes dlopen() fail with a recoverable
         * error here, instead of aborting the whole process later via the
         * PLT lazy-binding trampoline when the symbol is first called. */
        lib->dlhandle = dlopen(lib->out_path, RTLD_NOW);
        if (!lib->dlhandle) {
            fprintf(stderr, "rcc_lib: dlopen %s: %s\n", lib->out_path, dlerror());
            return NULL;
        }
        return dlsym(lib->dlhandle, name);
#endif
    }
#ifdef _WIN32
    return (void *)GetProcAddress((HMODULE)lib->dlhandle, name);
#else
    return dlsym(lib->dlhandle, name);
#endif
}

const char *rcc_lib_output_path(const RCCLib *lib) {
    return lib->out_path;
}

void rcc_lib_delete(RCCLib *lib) {
    if (!lib) return;
#ifdef _WIN32
    if (lib->dlhandle) FreeLibrary((HMODULE)lib->dlhandle);
#else
    if (lib->dlhandle) dlclose(lib->dlhandle);
#endif
    if (lib->asm_path) {
        unlink(lib->asm_path);
        free(lib->asm_path);
    }
    if (lib->out_path) {
        unlink(lib->out_path);
        free(lib->out_path);
    }
    free(lib);
}
