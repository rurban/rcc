/* `struct S { members... } __attribute__((packed));` — the packed
 * attribute trailing the *whole declaration*, after the closing brace —
 * was silently ignored entirely: neither member offsets nor the final
 * struct size were packed, identical to the fully-unpacked layout. The
 * leading form (`struct __attribute__((packed)) S {...}`) already
 * worked correctly.
 *
 * Root cause: struct_pack (the value that both the per-member offset
 * loop and the final size computation consult) is set once, from
 * struct_attr.is_packed, *before* a single member has been parsed —
 * but the trailing attribute's tokens don't exist yet at that point;
 * they come after the entire member list and the closing brace. By the
 * time the parser reaches them, every member's offset has already been
 * fixed using the unpacked layout.
 *
 * Found via a real Linux kernel build: arch/x86/include/asm/
 * alternative.h's struct alt_instr (used for the .altinstructions
 * section's entry size) is declared exactly this way. rcc's incorrect
 * sizeof (16 instead of the true 14) made the compiled .altinstructions
 * section's declared sh_entsize disagree with its actual packed byte
 * layout — objtool's own libelf-based rewrite step rejected the
 * resulting object file outright ("invalid section entry size").
 */
#include <stddef.h>

struct trailing_packed {
    int a;
    int b;
    unsigned u;
    unsigned char c;
    unsigned char d;
} __attribute__((packed));

static_assert(sizeof(struct trailing_packed) == 14,
              "trailing __attribute__((packed)) must pack the struct's own size");

/* The exact real-world shape that surfaced this: two 32-bit fields, a
 * union of a 2x16-bit bitfield struct and a plain u32, then two bytes —
 * arch/x86/include/asm/alternative.h's struct alt_instr. */
struct alt_instr_like {
    int instr_offset;
    int repl_offset;
    union {
        struct {
            unsigned cpuid : 16;
            unsigned flags : 16;
        };
        unsigned ft_flags;
    };
    unsigned char instrlen;
    unsigned char replacementlen;
} __attribute__((packed));

static_assert(sizeof(struct alt_instr_like) == 14,
              "packed struct with a union member must not gain trailing padding");

/* Packing must also close internal gaps, not just trim the tail. */
struct leading_gap {
    char a;
    int b;
} __attribute__((packed));

static_assert(sizeof(struct leading_gap) == 5,
              "trailing packed must close internal padding too, not just trailing");
static_assert(offsetof(struct leading_gap, b) == 1,
              "trailing packed must tightly pack member offsets");

int main(void)
{
    struct trailing_packed t = {1, 2, 3, 4, 5};
    if (t.a != 1 || t.b != 2 || t.u != 3 || t.c != 4 || t.d != 5) return 1;

    struct leading_gap g = {(char)7, 99};
    if (g.a != 7 || g.b != 99) return 2;

    return 0;
}
