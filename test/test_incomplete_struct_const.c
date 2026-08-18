/* A qualifier on an INCOMPLETE struct/union type (mimalloc.h's
 * `const mi_heap_t*` while `struct mi_heap_s` is still only
 * forward-declared) used to be stamped onto the SHARED tag type object,
 * permanently const-qualifying every later declaration of the same tag
 * in the translation unit. mimalloc's `mi_heap_t _mi_heap_main` in
 * src/init.c then read as const, and eval_const_expr()'s ND_MEMBER fold
 * (which correctly requires a const object) folded
 * `_mi_heap_main.thread_id == 0` to TRUE, compiling
 * `_mi_is_main_thread()` to `return 1` -- every worker thread then
 * shared _mi_heap_main and the multithreaded allocator corrupted
 * (mimalloc 2.1.2 `test-stress` SIGSEGV'd in a background thread).
 *
 * Root cause: declspec's quals block did `copy_type(ty)->qual |= quals`
 * for struct/union types, and copy_type() deliberately returns the
 * SHARED pointer for every struct/union (so a forward declaration can
 * still be completed later through every existing reference). The const
 * therefore landed on the canonical type, not on this one declaration.
 *
 * Fix: a qualified INCOMPLETE aggregate now gets its own "qualified
 * variant" (Type.qual_variants) linked off the canonical type, which
 * struct_or_union_specifier() completes in lockstep with the tag --
 * member access, sizeof and declaration-vs-definition type
 * compatibility all read through the variant, but its qualifier never
 * leaks onto the canonical type. A complete aggregate gets a plain
 * qualified copy (see qualify_struct_type in src/parser.c).
 *
 * This is the mimalloc pattern distilled: forward-declared tag,
 * typedef, a `const` use before completion, the completion, then a
 * NON-const global whose member reads must stay runtime reads. The
 * decl-before-completion / def-after-completion pair below also pins
 * the "conflicting types" regression (the variant must compare equal
 * to the completed type). Optimized builds only: the fold lives in the
 * -O1+ constant-expression evaluator.
 */

struct mi_heap_s;
typedef struct mi_heap_s mi_heap_t;

/* Declaration uses the still-incomplete const type; the definition
 * after the completion below must be a compatible redefinition. */
static const mi_heap_t *probe(const mi_heap_t *p);

struct mi_heap_s { long thread_id; };

static const mi_heap_t *probe(const mi_heap_t *p) { return p; }

static mi_heap_t _mi_heap_main = { 0 };

static int is_main_thread(void) {
    /* Folded to constant 1 when the shared type is wrongly const:
     * both reads then equal the static initializer 0. */
    return (_mi_heap_main.thread_id == 0) || (_mi_heap_main.thread_id == 7);
}

int main(void) {
    if (!is_main_thread())
        return 1; /* thread_id is 0 initially */
    _mi_heap_main.thread_id = 5;
    if (is_main_thread())
        return 2; /* 5 == 0 and 5 == 7 must both be false at runtime */
    if (probe(&_mi_heap_main)->thread_id != 5)
        return 3; /* member access through the completed variant */
    if (sizeof(const mi_heap_t) != sizeof(mi_heap_t))
        return 4;
    return 0;
}
