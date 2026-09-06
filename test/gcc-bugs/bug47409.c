/* GCC Bug #47409 - volatile struct member bug
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47409
 */
/* { dg-do compile } */


struct S { int a; volatile int b; long c; volatile char d[10]; char e[10]; };
struct S a, b;
volatile struct S c, d;
union U { int a; volatile char b[10]; volatile long c[5]; };
union U e, f;
volatile union U g, h;
struct T { int a; volatile int b; union U c; volatile union U d; };
struct T i, j;
volatile struct T k, l;
struct V { int a : 5; volatile int b : 7; volatile int c : 1; int d; volatile long e : 5; long f : 6; volatile long g : 1; long h : 1; };
struct V m, n;
volatile struct V o, p;
void f1 () { a = b; }
void f2 () { c = b; }
void f3 () { a = d; }
void f4 () { c = d; }
void f5 () { e = f; }
void f6 () { g = f; }
void f7 () { e = h; }
void f8 () { g = h; }
void f9 () { i = j; }
void f10 () { k = j; }
void f11 () { i = l; }
void f12 () { k = l; }
void f13 () { m = n; }
void f14 () { o = n; }
void f15 () { m = p; }
void f16 () { o = p; }
// I guess for struct S, it could gimplify it for f1 to:
// a.a = b.a;
// a.b = b.b;
// a.c = b.c;
// for (temp = 0; temp < 10; temp++)
// a.d[i] = b.d[i];
// a.e = b.e; // aggregate assignment
// and for f2, f3 and f4 the same, except that instead of the aggregate assignment
// at the end it would emit a loop similar to d field.
// But, what to do about unions?  The standard says that only one union member is active, but which one it is?  I think the compiler generally can't know.  So, do we just ignore unions and expand them always as we used to?  Pick up the first union member (or randomly or preferrably one with volatile)?
// What about bitfields?  Does it have to be per bitfield assignment, or can we e.g. assign the whole representative field at a time?
// What are other compilers doing here?
// I've tried clang 3.1, and don't see it would consider any of the volatile keywords here in any way.


