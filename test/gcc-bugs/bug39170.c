/* GCC Bug #39170 - provide an option to silence -Wconversion warnings for bit-fields
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39170
 */
/* { dg-do compile } */


struct S {
        unsigned int a : 3;
};

void f(struct S *s, int x)
{
        s->a = x; /* -Wconversion warns about bit-field truncation */
}
