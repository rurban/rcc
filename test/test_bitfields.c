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


    return 0;
}
