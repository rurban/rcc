/* `#pragma pack(N)` followed by a bare `#pragma pack()` (no push/pop/number)
 * must restore the compiler's default alignment for every struct declared
 * afterward — the standard "pack tightly, then go back to normal" idiom
 * every ACPI table header in the kernel uses (include/acpi/actbl.h,
 * actbl1.h, actbl2.h, actbl3.h, acrestyp.h: each wraps its packed table
 * structs in a single `#pragma pack(1)` / `#pragma pack()` pair).
 *
 * rcc's preprocessor recognized `#pragma pack(push[, N])` and
 * `#pragma pack(pop)`, and `#pragma pack(N)` for a literal 1-9, but the
 * bare reset form matched none of those three shapes and fell through to
 * a plain `return;` — never resetting the global pack_align state and
 * never emitting a marker for the parser to see. Once *any* header in a
 * translation unit used `#pragma pack(1)` ... `#pragma pack()`, every
 * struct declared afterward silently stayed packed to 1 byte, however far
 * away and however unrelated: found via a real Linux kernel build where
 * init/main.c (which includes <linux/acpi.h> transitively) failed
 * <linux/fs.h>'s `static_assert(offsetof(struct filename, iname) %
 * sizeof(long) == 0)` — struct filename lives thousands of lines after
 * the last ACPI header, with no packed attribute of its own, yet came out
 * 4 bytes short because the pointer member of an embedded anonymous
 * struct was left unpadded.
 */
#include <stddef.h>

struct pp_before_reset {
    char c;
    long l;
};
static_assert(sizeof(struct pp_before_reset) == sizeof(long) + sizeof(long),
              "sanity: default alignment pads before any #pragma pack");

#pragma pack(1)
struct pp_packed {
    char c;
    long l;
};
static_assert(sizeof(struct pp_packed) == sizeof(char) + sizeof(long),
              "sanity: #pragma pack(1) actually packs");
#pragma pack()

/* The real bug: this struct is declared *after* the pack(1)/pack() pair,
 * with no packed attribute of its own — it must get full default
 * alignment/padding, exactly like pp_before_reset above. */
struct pp_after_reset {
    char c;
    long l;
};
static_assert(sizeof(struct pp_after_reset) == sizeof(long) + sizeof(long),
              "#pragma pack() must restore default alignment, not leak pack(1)");
static_assert(offsetof(struct pp_after_reset, l) == sizeof(long),
              "#pragma pack() must restore default alignment, not leak pack(1)");

/* Same shape as the real trigger: a pointer member following a smaller
 * member, inside a struct declared well after the pack(1)/pack() pair. */
struct pp_ptr_after_reset {
    int i;
    void *p;
};
static_assert(offsetof(struct pp_ptr_after_reset, p) == sizeof(void *),
              "pointer member after #pragma pack() must be naturally aligned");

int main(void)
{
    struct pp_after_reset a = {0};
    if (offsetof(struct pp_after_reset, l) != sizeof(long)) return 1;
    if (sizeof(a) != sizeof(long) * 2) return 2;

    struct pp_ptr_after_reset b = {0};
    if (offsetof(struct pp_ptr_after_reset, p) != sizeof(void *)) return 3;

    return 0;
}
