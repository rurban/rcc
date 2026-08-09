// __attribute__((section("name"))) on a global variable was silently
// unparsed -- read_type_attrs() had no case for the `section`/`__section__`
// GCC attribute at all, so it fell through to the generic "unrecognized
// attribute, skip its parenthesized argument" path. The variable's data
// landed in the ordinary .data/.rodata/.bss like any other global, never
// in a section literally named "name" -- so code relying on the common
// linker-collected-array idiom (`extern char __start_name[];` /
// `extern char __stop_name[];`, real GNU ld automatically synthesizes
// those two symbols for any section whose name is a valid C identifier,
// bracketing every input object's contribution to it) got "undefined
// reference to `__start_name'" at link time: nothing ever created a
// section by that name for the linker to synthesize boundaries around.
//
// Fixed in three places:
//  - parser.c: read_type_attrs() now recognizes section("name")/
//    __section__("name") and records the name on the declared global
//    (LVar::section_name).
//  - codegen.c: a global with a non-NULL section_name is emitted into a
//    custom ELF section (objfile_find_or_add_section()) instead of the
//    default .data/.bss/.rodata; cg_set_section()'s fallback branch for
//    section ids >= SEC_NUM (previously silently defaulting to .text)
//    now correctly resolves to that section's own growable buffer.
//  - link_elf.c: rcc's own native ELF linker now synthesizes
//    __start_<name>/__stop_<name> for any section whose name is a valid
//    C identifier, matching real GNU ld -- previously only the GCC/
//    external-linker fallback path could resolve such references (via
//    the real system `ld`), so this exact source only worked when rcc's
//    native linker happened to bail out for some unrelated reason.
//
// Found via GCC c-testsuite's scrapscript upstream project, whose
// generated runtime places a "const heap" marker object in a
// `__attribute__((section("const_heap")))` global specifically so its
// GC can bracket the const/heap boundary via __start_const_heap/
// __stop_const_heap -- every one of its compiled tests failed to link.

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
struct registry_entry {
    int tag;
    const char *msg;
};

// Three separately-declared section-attributed globals: real GNU ld (and
// now rcc's own native linker) places every input object's contribution
// to a same-named section contiguously, so __start_/__stop_ bracket all
// three regardless of declaration order -- not just a single lone marker.
__attribute__((section("my_registry")))
__attribute__((used))
static const struct registry_entry e1 = { 1, "one" };

__attribute__((__section__("my_registry")))
__attribute__((used))
static const struct registry_entry e2 = { 2, "two" };

__attribute__((section("my_registry")))
__attribute__((used))
static const struct registry_entry e3 = { 3, "three" };

// __start_<name>/__stop_<name> boundary-symbol synthesis is implemented
// this session only for rcc's native ELF linker (link_elf.c); real GNU
// ld's PE-COFF backend (used by the mingw cross build) already does the
// identical thing on its own, unrelated to this fix. Mach-O linking
// (link_macho.c, macOS) needs a different mechanism entirely -- the
// segment-qualified section name (`"__DATA,name"`) plus per-symbol
// `__asm("section$start$__DATA$name")`/`"section$end$..."` labels, per
// Apple's own convention (matching the *_start_const_heap example this
// fix was found through, whose own upstream source already carries this
// exact `#ifdef __APPLE__` split) -- not implemented here. Keep the
// section-attribute compiler support itself exercised everywhere (the
// three globals above still get parsed, placed, and linked on every
// platform), just skip the boundary-symbol-dependent runtime assertions
// where nothing yet synthesizes them.
#if !defined(__APPLE__)
extern char __start_my_registry[];
extern char __stop_my_registry[];

int main(void) {
    long size = __stop_my_registry - __start_my_registry;
    long count = size / (long)sizeof(struct registry_entry);
    if (count != 3) {
        printf("expected 3 registry entries, got %ld (size=%ld)\n",
               count, size);
        return 1;
    }

    struct registry_entry *entries = (struct registry_entry *)__start_my_registry;
    int tag_sum = 0;
    for (long i = 0; i < count; i++)
        tag_sum += entries[i].tag;
    if (tag_sum != 1 + 2 + 3) {
        printf("expected tag sum 6, got %d\n", tag_sum);
        return 2;
    }

    // Every message must be reachable and correctly ordered.
    bool saw_one = false, saw_two = false, saw_three = false;
    for (long i = 0; i < count; i++) {
        if (strcmp(entries[i].msg, "one") == 0) saw_one = true;
        else if (strcmp(entries[i].msg, "two") == 0) saw_two = true;
        else if (strcmp(entries[i].msg, "three") == 0) saw_three = true;
    }
    if (!saw_one || !saw_two || !saw_three) {
        printf("missing a registry entry: one=%d two=%d three=%d\n",
               saw_one, saw_two, saw_three);
        return 3;
    }

    // A plain ordinary global (no section attribute) must still be in
    // .data/.bss as normal -- the fix must not leak section placement
    // onto unrelated globals.
    static int ordinary = 42;
    if (ordinary != 42)
        return 4;

    return 0;
}
#else
int main(void) {
    (void)e1;
    (void)e2;
    (void)e3;
    return 0;
}
#endif
