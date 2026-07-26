/* A union's anonymous struct/union member must still contribute its own
 * alignment to the enclosing union's overall alignment — same as it does
 * for the enclosing *struct*'s alignment (already covered by
 * test_struct_attr_align.c's socket_lock_t case, which nests a struct
 * inside a union but only checks *offsets*, not the union's own alignment).
 *
 * Regression: struct_or_union_specifier()'s two anonymous-member code
 * paths (untagged inline member, and the ordinary declarator-based one)
 * both updated `max_size` for a union's anonymous struct/union member but
 * never `max_align` — so a union whose only 8-byte-aligned content came
 * from *inside* an anonymous member (never a directly-named member of the
 * union itself) silently kept `max_align`'s initial value of 1. Two levels
 * of anonymous nesting (union directly containing an anonymous struct)
 * exposed it most severely, since neither level's alignment ever reached
 * the outermost type at all.
 *
 * Found via a real Linux kernel build: arch/x86/kernel/signal_64.c's
 * `static_assert(__alignof__(siginfo_t) == 8)` — asm-generic/siginfo.h's
 * siginfo_t is `struct { union { struct { ...; union __sifields _sifields;
 * }; int pad[...]; }; }`, and `union __sifields` contains a `void *`
 * several members deep. rcc computed __alignof__(siginfo_t) == 1.
 */

union u1 {
    struct {
        int a;
        void *p; /* only source of 8-byte alignment in this union */
    };
};
static_assert(__alignof__(union u1) == __alignof__(void *),
              "anonymous struct member must widen its union's alignment");

/* Two levels of anonymous nesting: a struct whose sole content is an
 * anonymous union directly containing an anonymous struct (the exact
 * siginfo_t shape, minus the unrelated surrounding fields). */
typedef struct {
    union {
        struct {
            int si_signo;
            void *si_ptr;
        };
    };
} siginfo_like_t;
static_assert(__alignof__(siginfo_like_t) == __alignof__(void *),
              "double anonymous nesting must still propagate alignment");

/* A second, same-shaped anonymous member sitting *beside* a plain `int`
 * array inside the union (matches siginfo_t's real "sifields vs pad[]"
 * shape exactly) must still win out over the array's narrower alignment. */
typedef struct {
    union {
        struct {
            int si_signo2;
            void *si_ptr2;
        };
        int pad[16];
    };
} siginfo_like2_t;
static_assert(__alignof__(siginfo_like2_t) == __alignof__(void *),
              "anonymous member alongside a wider-but-less-aligned sibling "
              "must still set the union's alignment");

int main(void) {
    union u1 v;
    v.p = (void *)0x1234;
    if (v.p != (void *)0x1234) return 1;

    siginfo_like_t s;
    s.si_signo = 7;
    if (s.si_signo != 7) return 2;

    siginfo_like2_t s2;
    s2.pad[0] = 42;
    if (s2.pad[0] != 42) return 3;

    return 0;
}
