/* __attribute__((weak)) on a global VARIABLE (not a function) was silently
 * dropped in two different, independent ways:
 *
 * 1. Parser (parser.c, declarator()): a *trailing* weak attribute right
 *    after the declared identifier -- `int x __attribute__((weak));` --
 *    was parsed into a local `trail_attr` struct and then simply never
 *    read; only the separate *pointer*-attribute case just above it
 *    (`int *p __attribute__((weak))`) propagated into `pending_weak`.
 *    (A *prefix* weak attribute, `__attribute__((weak)) int x;`, worked
 *    correctly via `attr.is_weak` -- once codegen's own gap, below, was
 *    also fixed.)
 * 2. Codegen (codegen.c, the prog->globals emission loop): even when
 *    `var->is_weak` WAS correctly set, the .bss/.data symbol-binding
 *    choice only ever checked `var->is_static` (SB_LOCAL vs SB_GLOBAL),
 *    never `var->is_weak` -- so a weak variable's own DEFINITION was
 *    still emitted with STB_GLOBAL, not STB_WEAK.
 *
 * Either gap alone reintroduces the real-world bug this fix targets:
 * golang/go's go1.4 bootstrap sources (test_go) declare, in a header
 * included by nearly every C file in lib9/libbio/liblink,
 *   #define AUTOLIB(x) int __p9l_autolib_ ## x __attribute__ ((weak));
 * -- used once per translation unit to "tip off 9l to autolink" a
 * library. Every one of those per-file symbols shares the same name; a
 * non-weak STB_GLOBAL binding collided as a hard "multiple definition"
 * link error building liblink.a/libbio.a's own archive members instead
 * of the silently-merged single definition weak linkage exists for.
 */
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

static int run(const char *rcc, const char *src, const char *obj) {
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, src, obj);
    return system(cmd);
}

#ifndef _WIN32
// Reads `obj`'s symbol table via `nm` and returns true if `sym_name` is
// bound weak. Two entirely different output shapes to detect it:
// GNU nm's default 3-field format (`<addr> <type> <name>`) marks a weak
// symbol with a single type char ('v'/'V'/'w'/'W'); Darwin's nm has no
// such letter in its default single-letter type column at all, so `-m`
// ("mnemonic"/verbose) is required there, printing a descriptive
// property string that spells out "weak external"/"weak private
// external" for a weak definition instead. Either way the symbol name
// is always the LAST whitespace-delimited token on the line -- Mach-O
// additionally prefixes every C symbol with an underscore, matched
// with or without it.
//
// PE/COFF (Windows) has no ELF/Mach-O-equivalent "weak definition"
// concept at all -- COFF's own IMAGE_SYM_CLASS_WEAK_EXTERNAL exists
// solely for an *unresolved* reference's fallback default, not a real
// storage-backed definition like this -- so this whole check (and the
// `nm` dependency) is skipped on Windows entirely; see
// test_asm_hidden_visibility.c's compile_and_check_visibility() for the
// same precedent skipping ELF-specific binding verification there.
static int is_weak_symbol(const char *obj, const char *sym_name) {
    char cmd[1400];
#ifdef __APPLE__
    snprintf(cmd, sizeof(cmd), "nm -m %s 2>/dev/null", obj);
#else
    snprintf(cmd, sizeof(cmd), "nm %s 2>/dev/null", obj);
#endif
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    char line[512];
    int found_weak = -1;
    while (fgets(line, sizeof(line), p)) {
        size_t llen = strlen(line);
        while (llen && (line[llen - 1] == '\n' || line[llen - 1] == '\r'))
            line[--llen] = '\0';
        if (!llen) continue;
        char *last_space = strrchr(line, ' ');
        const char *name_tok = last_space ? last_space + 1 : line;
        const char *bare = (name_tok[0] == '_') ? name_tok + 1 : name_tok;
        if (strcmp(name_tok, sym_name) != 0 && strcmp(bare, sym_name) != 0)
            continue;
#ifdef __APPLE__
        // Search only the descriptive-property portion of the line
        // (before the trailing name field) -- a plain substring search
        // over the whole line would false-positive on any symbol name
        // that itself happens to contain "weak". Manual search (not
        // memmem, a GNU/BSD extension not universally declared under
        // strict -std=c11) over the bounded prefix.
        size_t prefix_len = last_space ? (size_t)(last_space - line) : 0;
        found_weak = 0;
        for (size_t i = 0; i + 4 <= prefix_len; i++) {
            if (memcmp(line + i, "weak", 4) == 0) {
                found_weak = 1;
                break;
            }
        }
#else
        // Manual whitespace-token walk (not strtok_r, a POSIX function
        // that may not be declared under strict -std=c11 without an
        // explicit feature-test macro): GNU nm's line is exactly
        // `<addr> <type> <name>`; find the second token's start/end.
        char *t = line;
        while (*t == ' ' || *t == '\t') t++;
        while (*t && *t != ' ' && *t != '\t') t++; // skip addr token
        while (*t == ' ' || *t == '\t') t++;
        char *type_start = t;
        while (*t && *t != ' ' && *t != '\t') t++; // end of type token
        found_weak = 0;
        for (char *c = type_start; c < t; c++) {
            if (*c == 'v' || *c == 'V' || *c == 'w' || *c == 'W') {
                found_weak = 1;
                break;
            }
        }
#endif
    }
    pclose(p);
    return found_weak;
}
#endif // !_WIN32

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char src[600], obj[700];

    // Case 1: trailing weak attribute, tentative (.bss) definition --
    // the exact AUTOLIB() shape.
    snprintf(src, sizeof(src), "%s/test_weakvar1_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_weakvar1_%d.o", td, pid);
    if (!write_file_helper(src, "int __p9l_autolib_bio __attribute__((weak));\n"))
        return 1;
    if (run(rcc, src, obj) != 0) {
        printf("FAIL: case 1 failed to compile\n");
        return 2;
    }
#ifndef _WIN32
    int w1 = is_weak_symbol(obj, "__p9l_autolib_bio");
    if (w1 != 1) {
        remove(src);
        remove(obj);
        printf("FAIL: case 1 (trailing weak, tentative/.bss) not bound weak "
               "(found_weak=%d)\n", w1);
        return 3;
    }
#endif
    remove(src);
    remove(obj);

    // Case 2: trailing weak attribute, initialized (.data) definition.
    snprintf(src, sizeof(src), "%s/test_weakvar2_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_weakvar2_%d.o", td, pid);
    if (!write_file_helper(src, "int weakvar_data_x __attribute__((weak)) = 5;\n"))
        return 4;
    if (run(rcc, src, obj) != 0) {
        printf("FAIL: case 2 failed to compile\n");
        return 5;
    }
#ifndef _WIN32
    int w2 = is_weak_symbol(obj, "weakvar_data_x");
    if (w2 != 1) {
        remove(src);
        remove(obj);
        printf("FAIL: case 2 (trailing weak, initialized/.data) not bound "
               "weak (found_weak=%d)\n", w2);
        return 6;
    }
#endif
    remove(src);
    remove(obj);

    // Case 3: prefix weak attribute (leading, before the type) must
    // still work too -- the same codegen fix serves both spellings.
    snprintf(src, sizeof(src), "%s/test_weakvar3_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_weakvar3_%d.o", td, pid);
    if (!write_file_helper(src, "__attribute__((weak)) int weakvar_prefix_y;\n"))
        return 7;
    if (run(rcc, src, obj) != 0) {
        printf("FAIL: case 3 failed to compile\n");
        return 8;
    }
#ifndef _WIN32
    int w3 = is_weak_symbol(obj, "weakvar_prefix_y");
    if (w3 != 1) {
        remove(src);
        remove(obj);
        printf("FAIL: case 3 (prefix weak, tentative/.bss) not bound weak "
               "(found_weak=%d)\n", w3);
        return 9;
    }
#endif
    remove(src);
    remove(obj);

    // Case 4: an ordinary (non-weak) global variable must NOT become
    // weak as a side effect of this fix. (Named without "weak" as a
    // substring -- the Darwin nm -m weak check above scans the line's
    // descriptive-property prefix only, but keep the fixture itself
    // unambiguous too.)
    snprintf(src, sizeof(src), "%s/test_weakvar4_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_weakvar4_%d.o", td, pid);
    if (!write_file_helper(src, "int plainvar_ordinary_z;\n"))
        return 10;
    if (run(rcc, src, obj) != 0) {
        printf("FAIL: case 4 failed to compile\n");
        return 11;
    }
#ifndef _WIN32
    int w4 = is_weak_symbol(obj, "plainvar_ordinary_z");
    if (w4 != 0) {
        remove(src);
        remove(obj);
        printf("FAIL: case 4 (ordinary, non-weak) wrongly bound weak "
               "(found_weak=%d)\n", w4);
        return 12;
    }
#endif
    remove(src);
    remove(obj);

    printf("OK\n");
    return 0;
}
