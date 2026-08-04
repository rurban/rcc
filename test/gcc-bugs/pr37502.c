/* GCC Bug #37502 - no warning for always-false/true conditions due to too small bitfields
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37502
 */
/* { dg-do compile } */


volatile struct {
        unsigned char a: 1;
} bits;

unsigned char
// getfoo(void)
{
        while (bits.a < 3)
//                 /* wait */;
        return 42;
}
// GCC does not emit a warning with -Wall -Wextra yet happily generates an
// infinite loop since the while condition is always true due to the
// limited size of the bitfield.  I expected to get a warning similar to
// those when using a too small integer type.


