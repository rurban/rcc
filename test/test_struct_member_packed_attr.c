/* A GNU `__attribute__((packed))` written trailing an individual struct
 * MEMBER's own declarator (`uint32_t crc32 __attribute__((packed));` --
 * busybox's own idiom in archival/unzip.c for tightening one field's
 * placement without packing the whole struct) was silently discarded:
 * declarator()'s own trailing-attribute read (right after the member
 * name) captured it into a purely local `trail_attr` that only ever
 * checked `is_weak`/`is_transparent_union` -- `is_packed` had nowhere
 * to go and was dropped, so every such member kept its natural
 * (unpacked) alignment. This silently mis-sized structs across bit-
 * exact wire-format code: busybox's own `struct BUG { char
 * BUG_zip_header_must_be_26_bytes[... ? 1 : -1]; ... }` static-assert
 * idiom (offsetof/sizeof checks against ZIP_HEADER_LEN=26/
 * CDF_HEADER_LEN=42) caught the wrong layout as a compile-time array-
 * size error. Confirmed the exact offset/size mismatch directly against
 * real gcc. Fixed by having declarator() merge its trailing attribute's
 * `is_packed` back into the caller-supplied VarAttr (like it already
 * does for `is_weak` via a pending flag) and having struct-member layout
 * pass a real VarAttr through and apply `is_packed` to just that
 * member's own alignment -- never mutating the member's TYPE itself
 * (which may be a shared typedef, e.g. uint32_t, used unpacked
 * elsewhere) nor affecting sibling members.
 */
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

/* Mirrors busybox archival/unzip.c's zip_header_t exactly: five plain
 * uint16_t members, then three uint32_t members each individually
 * packed, then two more plain uint16_t members. */
typedef union {
    uint8_t raw[26];
    struct {
        uint16_t version;
        uint16_t zip_flags;
        uint16_t method;
        uint16_t modtime;
        uint16_t moddate;
        uint32_t crc32 __attribute__((packed));
        uint32_t cmpsize __attribute__((packed));
        uint32_t ucmpsize __attribute__((packed));
        uint16_t filename_len;
        uint16_t extra_len;
    } fmt __attribute__((packed));
} zip_header_t;

/* A packed member must not perturb the alignment/layout of an ordinary,
 * non-packed sibling type used elsewhere -- the fix must apply only to
 * THIS member, never mutate the shared uint32_t type itself. */
struct plain_u32_holder {
    uint16_t a;
    uint32_t b;
};

int main(void)
{
    if (sizeof(zip_header_t) != 26) {
        printf("FAIL: sizeof(zip_header_t) = %zu, expected 26\n", sizeof(zip_header_t));
        return 1;
    }
    if (offsetof(zip_header_t, fmt.crc32) != 10) {
        printf("FAIL: offsetof(crc32) = %zu, expected 10\n", offsetof(zip_header_t, fmt.crc32));
        return 2;
    }
    if (offsetof(zip_header_t, fmt.cmpsize) != 14) {
        printf("FAIL: offsetof(cmpsize) = %zu, expected 14\n", offsetof(zip_header_t, fmt.cmpsize));
        return 3;
    }
    if (offsetof(zip_header_t, fmt.ucmpsize) != 18) {
        printf("FAIL: offsetof(ucmpsize) = %zu, expected 18\n", offsetof(zip_header_t, fmt.ucmpsize));
        return 4;
    }
    if (offsetof(zip_header_t, fmt.extra_len) != 24) {
        printf("FAIL: offsetof(extra_len) = %zu, expected 24\n", offsetof(zip_header_t, fmt.extra_len));
        return 5;
    }

    /* uint32_t's normal (unpacked) alignment must be unaffected: b lands
     * at offset 4 (aligned), not 2 (as if uint32_t itself became packed). */
    if (offsetof(struct plain_u32_holder, b) != 4) {
        printf("FAIL: unpacked sibling struct's uint32_t offset = %zu, expected 4 "
               "(packed attribute leaked into the shared type)\n",
               offsetof(struct plain_u32_holder, b));
        return 6;
    }

    printf("OK\n");
    return 0;
}
