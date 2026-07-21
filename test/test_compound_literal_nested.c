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

    return 0;
}
