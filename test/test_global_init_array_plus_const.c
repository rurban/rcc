/* A pointer-typed global initializer computed from a bare array name
 * plus a constant offset (array-to-pointer decay, then pointer
 * arithmetic) -- "arr + N" or "arr + N + M + ..." -- must resolve via a
 * real relocation with the accumulated addend, not fail to parse at all.
 *
 * Regression: read_global_label_initializer() (the hand-rolled mini
 * parser used for pointer-typed global initializers) already handled a
 * trailing "+ const"/"- const" after a *string literal*, and a leading
 * "[index]"/".member" chain after a plain identifier, but never a
 * trailing "+ const" directly after a plain identifier (with or without
 * that chain) -- so it returned as if the initializer were already
 * complete right after the bare identifier, leaving the caller staring
 * at an unexpected "+" and reporting a confusing "expected ';' or ','"/
 * "expected specific operator" several tokens away from the real cause.
 *
 * Found via a real Linux kernel build: arch/x86/kernel/alternative.c's
 *   static const unsigned char x86nops[] = { ...11 NOP bytes... };
 *   const unsigned char * const x86_nops[ASM_NOP_MAX+1] = {
 *       NULL, x86nops, x86nops + 1, x86nops + 1 + 2, ...
 *   };
 */

static const unsigned char table[] = {10, 20, 30, 40, 50, 60};

static const unsigned char *const offsets[] = {
    table,
    table + 1,
    table + 1 + 2,
    table + 1 + 2 + 3,
};

/* Same idiom for a single (non-array-of-pointers) global. */
static const unsigned char *single = table + 2;

/* Non-byte-sized element type: the addend must be scaled by
 * sizeof(element), not treated as a raw byte count. Regression: the
 * "identifier + const" addend path added the constant unscaled, so
 * "struct_table + 1" landed 1 byte past struct_table instead of
 * sizeof(struct entry) bytes past it. Real-world case: PostgreSQL's
 * guc_tables.c `ssl_protocol_versions_info + 1` (an array of
 * `struct config_enum_entry`, 16 bytes each) used to look up enum
 * values starting from the wrong, misaligned base pointer.
 */
struct entry { const char *name; int val; _Bool hidden; };

static const struct entry struct_table[] = {
    {"zero", 0, 0},
    {"one", 1, 0},
    {"two", 2, 0},
    {0, 0, 0},
};

static const struct entry *const struct_offset = struct_table + 1;

int main(void) {
    if (offsets[0] != &table[0]) return 1;
    if (offsets[1] != &table[1]) return 2;
    if (offsets[2] != &table[3]) return 3;
    if (offsets[3] != &table[6]) return 4;
    if (single != &table[2]) return 5;
    if (struct_offset != &struct_table[1]) return 7;
    if (struct_offset->val != 1) return 8;
    if (*offsets[2] != 40) return 6;
    return 0;
}
