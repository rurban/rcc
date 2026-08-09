/* Struct-initializer gaps hit while building the Linux kernel with rcc:
 *
 * - (Type){...} compound literals only unrolled one level of nested
 *   struct/union member brace-initializers; a member that is itself a
 *   struct/union with its own (possibly redundantly over-braced) value
 *   failed to parse. Common via kernel wrapper-struct chains like
 *   atomic_t -> arch_spinlock_t -> raw_spinlock_t.
 *
 * - The compound-literal top-level designator loop used a flat scan that
 *   skipped anonymous struct/union members, so a designator only
 *   reachable through an anonymous member (like .ubuf below, mirroring
 *   struct iov_iter) was never found.
 *
 * - An EXPLICIT multi-level designator chain through a NAMED (non-
 *   anonymous) intermediate member -- e.g. ".bits.i" below, reaching a
 *   union member through its own named "bits" field -- resolved to the
 *   LEAF member's offset within its immediate parent (0, since every
 *   union member sits at offset 0 within the union) instead of that
 *   offset PLUS the intermediate "bits" member's own offset within the
 *   outer struct. The write landed at the wrong absolute offset,
 *   clobbering whichever earlier field (here ".type") happened to start
 *   at that same offset -- found via qbe's own `(Con){.type = CBits,
 *   .bits.i = val}` constant-table entries, which silently corrupted
 *   every constant qbe's own x86-64 backend created this way.
 */

struct inner { int val; };
struct middle { struct inner in; };
struct outer { struct middle mid; int tag; };

struct iov_iter_like {
    union {
        void *ubuf;
        int idx;
    };
    int flags;
};

/* Mirrors qbe's struct Con: an enum "type" tag followed by a NAMED
 * (not anonymous) union "bits" -- ".bits.i" must resolve through the
 * union member's own offset within Con, not just "i"'s offset (0)
 * within the union itself. */
enum { CUndef, CBits, CAddr };
struct Con {
    int type;
    union { long long i; double d; } bits;
    char flt;
};

/* Compound literal used as an initializer (local_init_one's designator
 * chain) -- already correct before this fix; kept as a control case. */
static struct Con con_init_form(long long v) {
    struct Con c = { .type = CBits, .bits.i = v };
    return c;
}

/* Compound literal used as a plain EXPRESSION (the buggy path: parser.c's
 * unary()-level "(type){...}" designator-chain flattening). */
static struct Con con_expr_form(long long v) {
    return (struct Con){ .type = CBits, .bits.i = v };
}

int main(void)
{
    /* Two levels of nested struct compound-literal brace init. */
    struct outer o = (struct outer){ { { 42 } }, 7 };
    if (o.mid.in.val != 42) return 1;
    if (o.tag != 7) return 2;

    /* A lone extra brace layer around a scalar leaf must still parse. */
    struct outer o2 = (struct outer){ .mid = { .in = { 99 } } };
    if (o2.mid.in.val != 99) return 3;

    /* Designator reaching a member only visible through an anonymous
     * union must be resolved. */
    struct iov_iter_like it = (struct iov_iter_like){ .ubuf = (void *)0x1234, .flags = 1 };
    if (it.ubuf != (void *)0x1234) return 4;
    if (it.flags != 1) return 5;

    /* Explicit ".bits.i" designator through a NAMED union member: both
     * the plain-initializer form and the expression-context compound
     * literal must land the value at "bits"'s offset within Con, not
     * offset 0 (aliasing "type"). */
    struct Con c1 = con_init_form(-80);
    if (c1.type != CBits) return 6;
    if (c1.bits.i != -80) return 7;

    struct Con c2 = con_expr_form(24);
    if (c2.type != CBits) return 8;
    if (c2.bits.i != 24) return 9;


    return 0;
}
