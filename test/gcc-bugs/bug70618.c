/* GCC Bug #70618 - better error messages for missing/too many arguments
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70618
 */
/* { dg-do compile } */


void foo(int *xp, float *yp, double *zp)
{
}

int x;
float y;
double z;
short k;

void f2(void)
{
        foo(&y, &z);    /* forgot x */ /* { dg-error "too few arguments" } */
        foo(&x, &z);    /* forgot y */ /* { dg-error "too few arguments" } */
        foo(&x, &z);    /* forgot z */ /* { dg-error "too few arguments" } */
        foo(&x);        /* forgot y and z */ /* { dg-error "too few arguments" } */
        foo(&z);        /* forgot x and y*/ /* { dg-error "too few arguments" } */

        foo(&x, &y, &z, &x);    /* x too many at end */ /* { dg-error "too many arguments" } */
        foo(&x, &x, &y, &z);    /* x too man at start */ /* { dg-error "too many arguments" } */
        foo(&x, &y, &y, &z);    /* y too much in the middle */ /* { dg-error "too many arguments" } */
        foo(&x, &y, &k, &z);    /* different y in middle */ /* { dg-error "too many arguments" } */
        foo(&k, &x, &y, &z);    /* different x at start */ /* { dg-error "too many arguments" } */
        foo(&x, &y, &z, &k);    /* different x at end */ /* { dg-error "too many arguments" } */
}
