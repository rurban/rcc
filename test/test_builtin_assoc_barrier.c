/* GCC __builtin_assoc_barrier(x) returns x unchanged but is an
 * optimization barrier (no FP reassociation/constant folding across it).
 * rcc previously had no such builtin: an unknown-call path took the
 * address of the (implicit-int) call result when a struct return value
 * was needed, failing with "lvalue required as left operand of
 * assignment" (GCC PR c/118869 exercises the struct case). Verify the
 * identity for scalar and struct operands.
 */
extern int printf(const char *, ...);
extern void abort(void);

struct S1 {
  unsigned char pad1;
  unsigned char val;
};

static double d;

static double fscale(double x)
{
    /* barrier on FP arg; rcc must not constant-fold/reassociate across */
    return __builtin_assoc_barrier(x) * 2.0;
}

static struct S1 fpass(struct S1 a)
{
    return __builtin_assoc_barrier(a);
}

int main(void)
{
    struct S1 x = {1, 2};
    struct S1 y = fpass(x);
    if (y.pad1 != 1 || y.val != 2)
        abort();

    d = 1.5;
    if (fscale(d) != 3.0)
        abort();

    printf("ok\n");
    return 0;
}
