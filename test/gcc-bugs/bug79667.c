/* GCC Bug #79667 - spurious -Wunused-variable on a local array of struct declared unused
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79667
 */
/* { dg-do compile } */


struct __attribute__ ((unused)) A { int i; };

void f (void)
{
  struct A a;      // no warning, ok
}

void g (void)
{ 
  struct A a[1];   // warning, bug
}
// + for lang in c c++
// + /build/gcc-git/gcc/xgcc -B /build/gcc-git/gcc -S -Wall -Wextra -Wunused -Wpedantic -xc y.C
// y.C: In function ‘g’:
// y.C:10:12: warning: unused variable ‘a’ [-Wunused-variable]
   struct A a[1];   // warning, bug
//             ^
// + for lang in c c++
// + /build/gcc-git/gcc/xgcc -B /build/gcc-git/gcc -S -Wall -Wextra -Wunused -Wpedantic -xc++ y.C
// y.C: In function ‘void g()’:
// y.C:10:12: warning: unused variable ‘a’ [-Wunused-variable]
   struct A a[1];   // warning, bug
//             ^


