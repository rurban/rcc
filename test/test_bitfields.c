static int test_pack_overflow_bitfield(void);

int printf(const char *, ...);

/* Register-allocator aliasing bug found via qbe (test/third_party/
 * test_qbe_simplecc): a ternary whose true-branch is a POSITIONAL
 * `(Ref){A, B}` compound literal for a struct with two sibling
 * bitfields, used as the LAST of 5 register-class call arguments
 * (2 plain ints + 3 struct-by-value args), corrupted the bitfield
 * merge. Assigning each sibling field does a read-modify-write: load
 * the word into `rt`, mask+shift the new value into `rv`, then
 * `rt |= rv`. Under the register pressure of 5 already-materialized
 * call arguments, allocating `rv` could steal `rt`'s (or the address
 * register `ra`'s) physical register outright -- not just spill it to
 * memory the way an ordinary outer/enclosing value would be, but
 * return the EXACT SAME index for the new VReg, since VReg identity
 * IS the physical register index here. The bitfield merge then
 * degenerated into a self-`or reg,reg` no-op (silently dropping the
 * new field's bits) or, in a narrower reproduction, corrupted the
 * store's own address register and segfaulted.
 */
typedef struct Ref { unsigned type : 3; unsigned val : 29; } Ref;

static Ref g_arg1;
static void emit5(int op, int k, Ref to, Ref arg0, Ref arg1) {
    (void)op; (void)k; (void)to; (void)arg0;
    g_arg1 = arg1;
}


struct bf {
    unsigned x : 3;
    unsigned y : 5;
    unsigned z : 8;
};

int main() {
    struct bf b = {1, 2, 3};
    printf("x=%d y=%d z=%d\n", b.x, b.y, b.z);

    b.x = 7;
    b.y = 15;
    b.z = 255;
    printf("x=%d y=%d z=%d\n", b.x, b.y, b.z);

    // Test compound assignment - THIS IS THE BUG WE'RE FIXING
    b.x = 3;
    printf("before +=: x=%d\n", b.x);
    b.x += 1; // 3 + 1 = 4 (should be 4, but was giving 0)
    printf("after +=1: x=%d\n", b.x);

    // Test bitfield with different types
    struct bf2 {
        char a : 4;
        short b : 8;
        int c : 16;
    } b2 = {1, 2, 3};
    printf("b2: a=%d b=%d c=%d\n", b2.a, b2.b, b2.c);

    // Ternary + positional compound literal as the last of 5 register
    // args: both fields of the true branch must land correctly, and
    // the false branch (plain struct var) must be unaffected either way.
    Ref r1 = {0, 10}, ap = {0, 5}, c4 = {0, 4};
    int isint = 1;
    emit5(5, 1, r1, ap, isint ? (Ref){1, 1} : c4);
    if (g_arg1.type != 1 || g_arg1.val != 1) return 6;
    isint = 0;
    emit5(5, 1, r1, ap, isint ? (Ref){1, 1} : c4);
    if (g_arg1.type != 0 || g_arg1.val != 4) return 7;


    int po = test_pack_overflow_bitfield();
    if (po) return 10 + po;
    return 0;
}

/* csmith fuzzing (test/csmith/1513742-{reduced_refmismatch_O0_22,
 * fail_refmismatch_{7,22,34}}.c): #pragma pack(1) struct S0 {
 * signed f0:25; unsigned f1:12; signed f2:31; uint64_t f3; } packs
 * f2 starting at bit 37, so its 31 bits need 5 bytes (bit_off=5,
 * needed=(5+31+7)/8=5) -- more than its declared "int" unit (4
 * bytes) -- and the dense-packing layout in parser.c correctly
 * widens mem->bf_load_size to 8 for this. But global_init_member()
 * (static-initializer writer) computed `unit_sz` from mem->ty->size
 * alone, ignoring bf_load_size: it read/wrote the bitfield's old/new
 * value through a 4-byte (uint32_t) window, silently truncating the
 * top 4 bits of f2's sign-extended value (bits 32..35 of the shifted
 * 64-bit new_val) instead of spilling into the 5th byte. Every
 * struct laid out with rcc's compiler read back f2 as a positive
 * garbage value (134208944) instead of -8784, and the byte at
 * offset+4 stayed 0x00 instead of gcc's 0x0f. Fixed by using
 * mem->bf_load_size (clamped to 8) as global_init_member's unit_sz,
 * matching the read/write codegen paths.
 */
#pragma pack(1)
struct __attribute__((gcc_struct)) PackOverflow {
    signed f0 : 25;
    unsigned f1 : 12;
    signed f2 : 31;
    unsigned long long f3;
};
#pragma pack()
static struct PackOverflow g_pack_overflow = {-4607, 49, -8784, 0};

static int test_pack_overflow_bitfield(void) {
    if (sizeof(struct PackOverflow) != 17) return 1;
    if (g_pack_overflow.f0 != -4607) return 2;
    if (g_pack_overflow.f1 != 49) return 3;
    if (g_pack_overflow.f2 != -8784) return 4; /* was 134208944 */
    if (g_pack_overflow.f3 != 0) return 5;
    unsigned char *p = (unsigned char *)&g_pack_overflow;
    if (p[8] != 0x0f) return 6; /* was 0x00: top nibble of f2 dropped */
    return 0;
}
