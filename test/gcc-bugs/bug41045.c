/* GCC Bug #41045 - Extended asm with C operands doesn’t work at top level
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=41045
 */
/* { dg-do compile } */


int a[2], b;
    enum { E1, E2, E3, E4, E5 };
    struct S { int a; char b; long long c; };
asm (".section blah; .quad %P0, %P1, %P2, %P3, %P4; .previous"
     : : "m" (a), "m" (b), "i" (42), "i" (E4), "i" (sizeof (struct S)));

