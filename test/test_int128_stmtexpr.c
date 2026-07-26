/* GNU statement expression `({ ...; last_expr; })` evaluated in __int128
 * context: gen_int128() previously had no ND_STMT_EXPR case, aborting with
 * "unsupported node kind" whenever a typeof()-based macro (the kernel's
 * min()/max()/READ_ONCE() style) was applied to a 128-bit operand. */

#define MIN(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#define MAX(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })

int main(void)
{
    __int128 a = 100, b = 42;

    __int128 m = MIN(a, b);
    if (m != 42) return 1;

    __int128 mx = MAX(a, b);
    if (mx != 100) return 2;

    unsigned __int128 x = 5, y = 9;
    unsigned __int128 mn = MIN(x, y);
    if (mn != 5) return 3;

    /* Statement expression with a side effect before the trailing value —
     * must still run for side effects, not just fold to the last expr. */
    int side = 0;
    __int128 v = ({ side++; a + b; });
    if (side != 1) return 4;
    if (v != 142) return 5;

    return 0;
}
