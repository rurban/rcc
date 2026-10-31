/* `#pragma pack(N)` must only affect struct definitions that follow it in
 * file order — never structs defined earlier.
 *
 * rcc's preprocessor runs a full scan of the whole translation unit
 * (including all includes) before parsing starts, and its `#pragma pack`
 * handler mutated the parser-owned global pack_align immediately at
 * preprocess time. By the time the parser began, pack_align held the
 * END-OF-FILE pack state, and every struct layout (struct_or_union_specifier
 * captures struct_pack = pack_align at the struct's start) used it instead
 * of the pack state active at its definition site. A file that packs
 * something late and leaves the pack active to EOF (libredwg's dwgbmp.c
 * does exactly this: `#pragma pack(1)` before its BMP header struct, never
 * restored) silently repacked EVERY struct in the TU — Dwg_Data shrank by
 * 176 bytes, its stack frame was undersized, and a memset of the full
 * sizeof clobbered the caller's return address (SEGV on old files with
 * thumbnails, even without -fsanitize).
 *
 * The preprocessor now only emits the pack marker tokens (from its own
 * shadow state, needed for push/pop math) and the parser resets pack_align
 * to 0 per file, reconstructing the position-dependent state from the
 * markers.
 */
#include <stddef.h>

/* Defined BEFORE any #pragma pack in this file: must keep its natural
 * layout no matter what pack state the file ends in. */
struct pp_before {
    char c;
    long l;
};
static_assert(sizeof(struct pp_before) == 2 * sizeof(long),
              "struct defined before a later #pragma pack must keep natural size");
static_assert(offsetof(struct pp_before, l) == sizeof(long),
              "member offset of a struct defined before a later #pragma pack");

/* The dwgbmp.c pattern: a packed struct later in the file, with the pack
 * state left active to the end of the translation unit. */
#pragma pack(1)
struct pp_packed {
    char c;
    long l;
};
static_assert(sizeof(struct pp_packed) == sizeof(char) + sizeof(long),
              "#pragma pack(1) must pack the struct defined at that point");

/* push/pop must still work after the fix (marker values come from the
 * preprocessor's shadow stack). */
#pragma pack(push, 4)
struct pp_push4 {
    char c;
    long l;
};
static_assert(sizeof(struct pp_push4) == sizeof(char) + 3 + sizeof(long),
              "pack(push, 4) limits the long member's alignment to 4");
#pragma pack(pop)

int main(void)
{
    struct pp_before b;
    struct pp_packed p;
    struct pp_push4 s;
    b.c = p.c = s.c = 0;
    if (sizeof(b) != 2 * sizeof(long)) return 1;
    if (offsetof(struct pp_before, l) != sizeof(long)) return 2;
    if (sizeof(p) != sizeof(char) + sizeof(long)) return 3;
    if (sizeof(s) != sizeof(char) + 3 + sizeof(long)) return 4;
    if (offsetof(struct pp_push4, l) != 4) return 5;
    return 0;
}
