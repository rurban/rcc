/* Three stacked bugs found while chasing test_ocaml/test_mimalloc's real
 * link failures (see test/third_party/TODO.md):
 *
 * 1. parser.c: a global variable's *definition* (tentative, e.g. `int x;`,
 *    or a real `__thread`/`_Thread_local` TLS definition) followed by a
 *    later `extern` redeclaration of the *same* name in the *same* TU
 *    unconditionally re-stamped `var->is_extern = attr.is_extern` whenever
 *    `!var->has_init` -- which is also true for a plain tentative
 *    definition (has_init is only set by an explicit `= value`
 *    initializer), so `int x; extern int x;` silently downgraded the
 *    definition back to a bare declaration and DROPPED the symbol from the
 *    object file entirely (no OBJECT/TLS symbol emitted at all -- not even
 *    weak or local). C11 6.9.2 requires the entity to remain defined.
 *    Real-world trigger: OCaml's `runtime/caml/domain_state.h` declares
 *    `extern __thread caml_domain_state* caml_state;`, included by
 *    `domain.c` *before* its own `CAMLexport CAMLthread_local
 *    caml_domain_state* caml_state;` definition -- extern-then-def, which
 *    already worked. The actual failure needed both this bug and #2 below;
 *    reproduced directly here as `int i; extern int i;` and the TLS
 *    equivalent, matching the general form of the underlying rule.
 *
 * 2. codegen.c (`asm_lea_tpoff_base_reg`, x86-64 non-PIC/local-exec TLS
 *    branch): a first *reference* (extern declaration only, no local
 *    definition) to a `__thread`/`_Thread_local` variable registered its
 *    new undefined symbol as plain `ST_NOTYPE` instead of `ST_TLS` --
 *    the sibling PIC/initial-exec branch just above it got this right
 *    (`ST_TLS`), only the non-PIC TPOFF32 path was wrong. The generated
 *    *code* was always correct (a real `%fs:`-relative TPOFF32 access);
 *    only the object file's own symbol-table *type* was wrong. This is
 *    invisible compiling a single TU alone -- it only surfaces once the
 *    linker cross-checks this object's undefined-reference symbol type
 *    against another object's real STT_TLS definition of the same name,
 *    exactly OCaml's `libcamlrun.a(alloc.b.o)` (reference-only, compiled
 *    without -fPIC) vs. `libcamlrun.a(domain.b.o)` (owns the definition):
 *    `ld` hard error "TLS definition ... mismatches non-TLS reference".
 *
 * 3. cg_builtins.c: `__builtin_thread_pointer()` (real GCC/Clang builtin,
 *    used by mimalloc's `_mi_prim_thread_id()` fast path) was entirely
 *    unimplemented -- `mimalloc-test-stress`'s TLS-heavy multithreaded
 *    build failed outright. Fixed by emitting `mrs x{r}, tpidr_el0`
 *    (ARM64) / `mov %fs:0, r` (x86-64 non-Windows -- real GCC does not
 *    support this builtin on mingw at all, verified against
 *    x86_64-w64-mingw32-gcc, so it stays unguarded/undefined there too).
 *
 * This test drives rcc as a subprocess (two separately compiled .o files
 * exercising bug #1 and #2, cross-linked exactly like a real archive) and
 * a third case exercising bug #3, since all three need real linking /
 * real codegen inspection -- not just "compiles and runs" in one TU. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static int write_file_helper(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(contents, f);
    fclose(f);
    return 1;
}

static int run_compile(const char *rcc, const char *extra_flags, const char *src, const char *obj) {
    char cmd[1400];
    snprintf(cmd, sizeof(cmd), "%s %s -c %s -o %s " NULL_REDIRECT, rcc, extra_flags, src, obj);
    return system(cmd);
}

#if !defined(_WIN32) && !defined(__APPLE__)
// Returns the ELF symbol TYPE letter readelf reports for the *last*
// matching row (TLS -> 't'/'T'-shaped "TLS" text token; readelf -sW's
// Type column literally prints "TLS", "OBJECT", "NOTYPE", "FUNC" etc.),
// or NULL if never found. Windows has no ELF symbol table to inspect,
// and Darwin's objects are Mach-O (no `readelf`, and TLS uses a
// completely different TLV-descriptor mechanism -- see codegen.c's
// `#if defined(__APPLE__)` TLVP branch, untouched by this fix) -- this
// whole check (and the other bugs it targets, which are ELF/TLS
// -fPIC-specific) is skipped on both, same precedent as
// test_weak_variable_attribute.c's is_weak_symbol().
static int find_sym_type(const char *obj, const char *sym_name, char *type_out, size_t type_out_sz) {
    char cmd[1400];
    snprintf(cmd, sizeof(cmd), "readelf -sW %s 2>/dev/null", obj);
    FILE *p = popen(cmd, "r");
    if (!p) return 0;
    char line[512];
    int found = 0;
    while (fgets(line, sizeof(line), p)) {
        // readelf -sW row: Num: Value Size Type Bind Vis Ndx Name
        char *last_space = strrchr(line, ' ');
        if (!last_space) continue;
        char name_buf[256];
        size_t llen = strlen(line);
        while (llen && (line[llen - 1] == '\n' || line[llen - 1] == '\r')) line[--llen] = '\0';
        char *nm = strrchr(line, ' ');
        if (!nm) continue;
        nm++;
        if (strcmp(nm, sym_name) != 0) continue;
        // Tokenize to find the 4th field (Type)
        char copy[512];
        strncpy(copy, line, sizeof(copy) - 1);
        copy[sizeof(copy) - 1] = '\0';
        char *tok = strtok(copy, " \t");
        int field = 0;
        while (tok) {
            field++;
            if (field == 4) {
                strncpy(type_out, tok, type_out_sz - 1);
                type_out[type_out_sz - 1] = '\0';
                found = 1;
                break;
            }
            tok = strtok(NULL, " \t");
        }
        (void)name_buf;
    }
    pclose(p);
    return found;
}
#endif

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char src[600], obj[700];

    // Case 1 (bug #1, plain global): `int i; extern int i;` in one TU
    // must still emit a real OBJECT definition, not drop the symbol.
    snprintf(src, sizeof(src), "%s/test_gerl1_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_gerl1_%d.o", td, pid);
    if (!write_file_helper(src, "int gerl_plain_i;\nextern int gerl_plain_i;\n"))
        return 1;
    if (run_compile(rcc, "", src, obj) != 0) {
        printf("FAIL: case 1 failed to compile\n");
        return 2;
    }
#if !defined(_WIN32) && !defined(__APPLE__)
    char type1[64] = "";
    int have1 = find_sym_type(obj, "gerl_plain_i", type1, sizeof(type1));
    if (!have1 || strcmp(type1, "OBJECT") != 0) {
        remove(src);
        remove(obj);
        printf("FAIL: case 1 (def then extern redecl) symbol missing or "
               "wrong type (have=%d type=%s)\n", have1, type1);
        return 3;
    }
#endif
    remove(src);
    remove(obj);

    // Case 2 (bug #1, TLS global): `_Thread_local int i; extern
    // _Thread_local int i;` must keep the TLS definition too.
    snprintf(src, sizeof(src), "%s/test_gerl2_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_gerl2_%d.o", td, pid);
    if (!write_file_helper(src, "_Thread_local int gerl_tls_i;\nextern _Thread_local int gerl_tls_i;\n"))
        return 4;
    if (run_compile(rcc, "", src, obj) != 0) {
        printf("FAIL: case 2 failed to compile\n");
        return 5;
    }
#if !defined(_WIN32) && !defined(__APPLE__)
    char type2[64] = "";
    int have2 = find_sym_type(obj, "gerl_tls_i", type2, sizeof(type2));
    if (!have2 || strcmp(type2, "TLS") != 0) {
        remove(src);
        remove(obj);
        printf("FAIL: case 2 (TLS def then extern redecl) symbol missing "
               "or wrong type (have=%d type=%s)\n", have2, type2);
        return 6;
    }
#endif
    remove(src);
    remove(obj);

    // Case 3 (bug #2): a *reference-only* TU (extern _Thread_local decl,
    // no local definition, compiled WITHOUT -fPIC -- the local-exec
    // model) must still register its undefined symbol as TLS-typed, not
    // NOTYPE, or a real linker rejects it against another TU's genuine
    // STT_TLS definition ("TLS definition ... mismatches non-TLS
    // reference").
    snprintf(src, sizeof(src), "%s/test_gerl3_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_gerl3_%d.o", td, pid);
    if (!write_file_helper(src,
            "extern _Thread_local int gerl_tls_ref;\n"
            "int gerl_get(void) { return gerl_tls_ref; }\n"
            "void gerl_set(int v) { gerl_tls_ref = v; }\n"))
        return 7;
    if (run_compile(rcc, "", src, obj) != 0) {
        printf("FAIL: case 3 failed to compile\n");
        return 8;
    }
#if !defined(_WIN32) && !defined(__APPLE__)
    char type3[64] = "";
    int have3 = find_sym_type(obj, "gerl_tls_ref", type3, sizeof(type3));
    if (!have3 || strcmp(type3, "TLS") != 0) {
        remove(src);
        remove(obj);
        printf("FAIL: case 3 (local-exec extern-only TLS reference) wrong "
               "symbol type (have=%d type=%s, expected TLS)\n", have3, type3);
        return 9;
    }
#endif
#if defined(_WIN32) || defined(__APPLE__)
    // The end-to-end link block below (which normally consumes and
    // removes this object) is skipped on this platform -- clean up here
    // instead.
    remove(src);
    remove(obj);
#endif
#if !defined(_WIN32) && !defined(__APPLE__)
    // End-to-end: link the definition (case 2's object, still on disk --
    // remove after) against this reference-only object plus a small
    // main(), exactly the real ocamlrun archive-link shape (no -fPIC,
    // non-PIE executable), and verify the value round-trips through TLS.
    // ELF-specific (exercises the exact local-exec TPOFF32 symbol-type
    // bug fixed above) -- Windows/PE has an entirely separate TLS
    // codegen path (`emit_emutls_addr`, untouched by this fix).
    snprintf(src, sizeof(src), "%s/test_gerl2_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_gerl2_%d.o", td, pid);
    if (!write_file_helper(src, "_Thread_local int gerl_tls_ref;\n"))
        return 10;
    char obj_def[700];
    snprintf(obj_def, sizeof(obj_def), "%s/test_gerl2_%d.o", td, pid);
    if (run_compile(rcc, "", src, obj_def) != 0) {
        printf("FAIL: link case failed to compile the definition TU\n");
        return 11;
    }
    char main_src[700], main_obj[700], exe[700];
    snprintf(main_src, sizeof(main_src), "%s/test_gerl_main_%d.c", td, pid);
    snprintf(main_obj, sizeof(main_obj), "%s/test_gerl_main_%d.o", td, pid);
    snprintf(exe, sizeof(exe), "%s/test_gerl_exe_%d", td, pid);
    if (!write_file_helper(main_src,
            "int gerl_get(void);\nvoid gerl_set(int);\n"
            "int main(void) { gerl_set(42); return gerl_get() == 42 ? 0 : 1; }\n"))
        return 12;
    if (run_compile(rcc, "", main_src, main_obj) != 0) {
        printf("FAIL: link case failed to compile main\n");
        return 13;
    }
    char obj3[700];
    snprintf(obj3, sizeof(obj3), "%s/test_gerl3_%d.o", td, pid);
    char link_cmd[2400];
    snprintf(link_cmd, sizeof(link_cmd), "%s -no-pie -o %s %s %s %s " NULL_REDIRECT,
             rcc, exe, main_obj, obj3, obj_def);
    int link_rc = system(link_cmd);
    remove(src);
    remove(obj_def);
    remove(main_src);
    remove(main_obj);
    remove(obj3);
    if (link_rc != 0) {
        remove(exe);
        printf("FAIL: end-to-end TLS link failed (rc=%d)\n", link_rc);
        return 14;
    }
    char run_cmd[800];
    snprintf(run_cmd, sizeof(run_cmd), "%s", exe);
    int run_rc = system(run_cmd);
    remove(exe);
    if (run_rc != 0) {
        printf("FAIL: end-to-end TLS round-trip returned wrong value (rc=%d)\n", run_rc);
        return 15;
    }
#endif

    // Case 4 (bug #3): __builtin_thread_pointer() must compile and
    // return a real, non-null, stable-within-thread pointer.
    snprintf(src, sizeof(src), "%s/test_gerl4_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_gerl4_%d.o", td, pid);
    if (!write_file_helper(src,
            "void *gerl_tp(void) { return __builtin_thread_pointer(); }\n"))
        return 16;
    if (run_compile(rcc, "", src, obj) != 0) {
        printf("FAIL: case 4 (__builtin_thread_pointer) failed to compile\n");
        return 17;
    }
    remove(src);
    remove(obj);

    printf("OK\n");
    return 0;
}
