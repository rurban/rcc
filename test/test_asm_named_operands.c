/* GCC extended-asm named operands: `[name] "constraint" (expr)` lets a
 * template reference an operand as %[name] instead of a positional %N, so
 * inserting/reordering operands doesn't require renumbering every
 * reference. Used pervasively in modern kernel inline asm (e.g.
 * `[errout] "+r" (err)` referenced as %[errout]).
 */
int main(void)
{
    int out = 0;
    int in = 42;
    __asm__ volatile ("movl %[in], %[out]"
                       : [out] "=r"(out)
                       : [in] "r"(in));
    if (out != 42) return 1;

    /* Mixed named output + positional input, and a name reused with a
     * size modifier (kernel style, e.g. %k[sel]). */
    long wide = 0x1122334455667788L;
    unsigned r = 0;
    __asm__ volatile ("movl %k[val], %[dst]"
                       : [dst] "=r"(r)
                       : [val] "r"(wide));
    if (r != 0x55667788u) return 2;

    return 0;
}
