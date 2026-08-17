/* C11 6.7.6.3p7: a function parameter declared with array type is
 * adjusted ("decays") to a pointer to the array's element type. The
 * modern prototype-style parameter parser (declarator_params()) already
 * applied this; the old-style K&R parameter-declaration-list parser
 * (parse_kr_param_list(), used for `void g(x) int x[][4]; { ... }`
 * definitions) did not -- it stored the raw, undecayed array type
 * verbatim. A K&R array parameter therefore kept a genuine (incomplete,
 * size-0) array type instead of a pointer: every `x[i][j]` index
 * computation used the wrong element stride, and the parameter's own
 * ABI slot was wrong (arrays are never passed by value at all). Found
 * via cc65's own LCC-derived K&R test corpus (test/ref/array.c, part of
 * test/third_party's test_cc65): a 2D-array K&R parameter produced
 * garbage output instead of the expected values.
 */
extern int printf(const char *, ...);

/* Old-style (K&R) single 1D array parameter. */
static void sum1d(x)
int x[];
{
    int i, total = 0;
    for (i = 0; i < 5; i++) total += x[i];
    printf("%d\n", total);
}

/* Old-style 2D array parameter (array-of-array-of-int) combined with an
 * array-of-pointer parameter in the same comma-separated K&R
 * declaration -- the exact shape cc65's test/ref/array.c uses. */
static int g_total;
static void scan2d(x, y)
int x[][4], *y[];
{
    int i, j;
    g_total = 0;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 4; j++)
            g_total += x[i][j];
    for (i = 0; i < 3; i++)
        g_total += y[i][0];
}

int main(void)
{
    int a[5] = {1, 2, 3, 4, 5};
    int z[3][4] = {{0, 1, 2, 3}, {10, 11, 12, 13}, {20, 21, 22, 23}};
    int row0[1] = {100}, row1[1] = {200}, row2[1] = {300};
    int *y[3];

    sum1d(a); /* prints 15; not itself checked, just must not crash */

    y[0] = row0;
    y[1] = row1;
    y[2] = row2;
    scan2d(z, y);

    /* sum of z (0+1+...+23 laid out above) = 138, plus y[i][0] sums to
     * 100+200+300 = 600 -> 738. A wrong (undecayed) array parameter type
     * corrupts the stride/ABI slot and produces a very different total. */
    if (g_total != 738) return 1;

    return 0;
}
