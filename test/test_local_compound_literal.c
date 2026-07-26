/* Named struct compound literals at function scope (e.g. (pgprot_t){x})
 * were incorrectly required to have a constant-expression initializer.
 * For local variables the initialization happens at runtime via codegen,
 * so a non-constant expression must be allowed (fixes pgtable_types.h
 * kernel errors). Global/static compound literals still require a
 * constant expression. */

typedef struct { unsigned long val; } pgprot_t;

static pgprot_t make_pgprot(unsigned long x)
{
    return (pgprot_t){x}; /* x is a function parameter, not a constant */
}

int main(void)
{
    unsigned long v = 5;
    pgprot_t p = (pgprot_t){v * 3}; /* non-constant initializer, local scope */
    if (p.val != 15) return 1;

    pgprot_t p2 = make_pgprot(7);
    if (p2.val != 7) return 2;

    return 0;
}
