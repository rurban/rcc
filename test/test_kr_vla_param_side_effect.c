/* C11 6.7.6.3p7: in a parameter declaration the size expression of a
 * VLA-typed parameter (incl. [static ...]) must be evaluated at each
 * function call, so its side effects are observable. The modern
 * prototype-style parser stores the dimension expression and emits it as
 * a statement at function entry; the old-style K&R parser
 * (parse_kr_param_list()) decayed the VLA parameter to a pointer but
 * dropped the dimension expression entirely, so e.g.
 * `void crime(s, c) char *s; char c[static func()]; {}` never called
 * func() at all (GCC PR c/105180; regression gcc 4.7+, rcc had the same
 * bug). Verify the side effect runs once per call.
 */
extern int printf(const char *, ...);
extern void abort(void);

static int global = 0;

static int func(void)
{
    global++;
    return global;
}

/* Old-style (K&R) definition whose parameter's [static func()] size
 * expression must be evaluated on every call. */
static void crime(s, c)
char *s;
char c[static func()];
{
    (void)s;
    (void)c;
}

static void k_via_kr(void)
{
    crime("1", "1");
    crime("1", "1");
    crime("1", "1");
}

int main(void)
{
    crime("1", "1");
    if (global != 1)
        abort();

    k_via_kr();
    if (global != 4) /* 1 + one evaluation per each of the 3 calls */
        abort();

    printf("ok: %d\n", global);
    return 0;
}
