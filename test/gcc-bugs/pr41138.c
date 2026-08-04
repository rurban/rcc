/* GCC Bug #41138 - Inconsistent (incorrect?) "overflow in implicit constant conversion" warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=41138
 */


struct S {
    unsigned a:1;
    unsigned b:1;
};

void f (struct S *s, int i) {
//     s->a = i & 0x10;   /* line 7: no warning */
//     s->b = i & 0x80;   /* line 8: warning  */
}
// EOF
// <stdin>: In function ‘f’:
// <stdin>:8:5: warning: overflow in implicit constant conversion [-Woverflow]


