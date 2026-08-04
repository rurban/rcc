/* GCC Bug #113973 - Please issue a warning when using plain character values in bitwise operations
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113973
 */

/* Enhancement proposal: warn when a (signed) char value is promoted to
 * int and used in bitwise ops where more than 8 bits of the sign-extended
 * result are used.  The expression below is the motivating case. */
static char x = 0xD8;

int main(void)
{
    // sign-extends x to 0xFFFFFFD8 before the or
    int y = 0x1200 | x; /* { dg-warning "sign-extends x" } */
    //assert(y == 216);
    return y;
}
