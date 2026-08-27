/* Regression test: a string literal cast to an integer type in a static
 * initializer must store the literal's ADDRESS, not its truthiness.
 *
 * eval_const_expr() folds ND_STR to 1 ("a string address is never null"),
 * which is correct for truthiness contexts (ternary/logical conditions)
 * but wrong when the fold result is used as the stored VALUE of an
 * integer-typed scalar initializer. Both global-initializer call sites
 * ran the plain integer const-eval BEFORE their address-reloc fallback,
 * so `intptr_t g = (intptr_t)"all";` and a struct's
 * `.defval = (intptr_t)"all"` (git's `struct option` tables) silently
 * stored 1 instead of a relocation to the literal.
 *
 * Found via test_git: t1013-read-tree-submodule.sh -- git's
 * `status -u -s` segfaulted in git_parse_maybe_bool_text() because the
 * -u option entry's defval ("all") was the pointer 0x1, and
 * parse_untracked_setting_name() parsed that address as the untracked-
 * files mode. Fixed by moving the existing looks_like_address_expr() +
 * extract_reloc() address-reloc fallback ABOVE the plain
 * eval_const_expr() integer-const path in global_init_one() and
 * global_initializer_impl().
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static intptr_t g1 = (intptr_t)"all";          /* scalar, cast, file scope */
static unsigned long long g2 = (unsigned long long)"mode";
static long long g3 = (long long)"x";

/* Struct-member variant, exactly git's struct option shape: the
 * pointer-typed long_name must be a reloc, and the intptr_t defval must
 * hold the string address, not 1. */
struct opt {
    int type;
    int short_name;
    const char *long_name;
    void *value;
    size_t precision;
    const char *argh;
    const char *help;
    int flags;
    void *callback;
    intptr_t defval;
};
static struct opt o = {
    .type = 10,
    .short_name = 'u',
    .long_name = "untracked-files",
    .value = (void *)0x1234,
    .argh = "mode",
    .help = "show untracked files",
    .flags = 1,
    .defval = (intptr_t)"all",
};

/* Existing static-pointer-int behavior must not regress. */
static void *p = (void *)0xdeadbeef;           /* int32 overflow, no sign-extend */
static char *q = (char *)0xffffffff;
static void *r = (void *)-1;
static long d = (long)0xdeadbeef;

int main(void) {
    /* The casts must yield the literals' addresses, not 1. */
    assert(g1 && strcmp((const char *)g1, "all") == 0);
    assert(g2 && strcmp((const char *)g2, "mode") == 0);
    assert(g3 && strcmp((const char *)g3, "x") == 0);

    assert(o.type == 10);
    assert(o.short_name == 'u');
    assert(o.long_name && strcmp(o.long_name, "untracked-files") == 0);
    assert(o.value == (void *)0x1234);
    assert(o.argh && strcmp(o.argh, "mode") == 0);
    assert(o.help && strcmp(o.help, "show untracked files") == 0);
    assert(o.flags == 1);
    assert(o.defval && strcmp((const char *)o.defval, "all") == 0);

    assert(p == (void *)(unsigned long long)0x00000000deadbeefULL);
    assert(q == (void *)(unsigned long long)0x00000000ffffffffULL);
    assert(r == (void *)-1);
    assert(d == 0xdeadbeefL);

    /* g1/g2/g3/o.defval must be distinct, real rodata addresses -- never
     * the truthiness constant 1. */
    assert(g1 != 1 && g2 != 1 && g3 != 1 && o.defval != 1);

    printf("ok\n");
    return 0;
}
