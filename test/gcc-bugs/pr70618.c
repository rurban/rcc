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
//         foo(&y, &z);    /* forgot x */
//         foo(&x, &z);    /* forgot y */
//         foo(&x, &z);    /* forgot z */
//         foo(&x);        /* forgot y and z */
//         foo(&z);        /* forgot x and y*/

//         foo(&x, &y, &z, &x);    /* x too many at end */
//         foo(&x, &x, &y, &z);    /* x too man at start */
//         foo(&x, &y, &y, &z);    /* y too much in the middle */
//         foo(&x, &y, &k, &z);    /* different y in middle */
//         foo(&k, &x, &y, &z);    /* different x at start */
//         foo(&x, &y, &z, &k);    /* different x at end */
}
// gcc/tsrc/tmissing.c: In function ‘f2’:
// gcc/tsrc/tmissing.c:14:6: warning: passing argument 1 of ‘foo’ from incompatible pointer type [-Wincompatible-pointer-types]
//   foo(&y, &z);  /* forgot x */
//       ^
// gcc/tsrc/tmissing.c:3:6: note: expected ‘int *’ but argument is of type ‘float *’
 void foo(int *xp, float *yp, double *zp)
//       ^
// gcc/tsrc/tmissing.c:14:10: warning: passing argument 2 of ‘foo’ from incompatible pointer type [-Wincompatible-pointer-types]
//   foo(&y, &z);  /* forgot x */
//           ^
// gcc/tsrc/tmissing.c:3:6: note: expected ‘float *’ but argument is of type ‘double *’
 void foo(int *xp, float *yp, double *zp)
//       ^
// gcc/tsrc/tmissing.c:14:2: error: too few arguments to function ‘foo’
//   foo(&y, &z);  /* forgot x */
//   ^
// gcc/tsrc/tmissing.c:3:6: note: declared here
 void foo(int *xp, float *yp, double *zp)
//       ^
// gcc/tsrc/tmissing.c:15:10: warning: passing argument 2 of ‘foo’ from incompatible pointer type [-Wincompatible-pointer-types]
//   foo(&x, &z); /* forgot y */
//           ^
// gcc/tsrc/tmissing.c:3:6: note: expected ‘float *’ but argument is of type ‘double *’
 void foo(int *xp, float *yp, double *zp)
//       ^
// gcc/tsrc/tmissing.c:15:2: error: too few arguments to function ‘foo’
//   foo(&x, &z); /* forgot y */


