/* GCC Bug #79074 - -Waddress difference between C and C++ with (T*)0
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79074
 */
/* { dg-do compile } */


int f (int i) { return &i != (void *) 0; }

int g (int i) { return &i != (int *) 0; }
// + for l in c c++
// + /build/gcc-svn/gcc/xgcc -B /build/gcc-svn/gcc -S -Waddress -xc t.c
// t.c: In function ‘f’:
// t.c:1:27: warning: the comparison will always evaluate as ‘true’ for the address of ‘i’ will never be NULL [-Waddress]
 int f (int i) { return &i != (void *) 0; }
//                            ^~
// + for l in c c++
// + /build/gcc-svn/gcc/xgcc -B /build/gcc-svn/gcc -S -Waddress -xc++ t.c
// t.c: In function ‘int f(int)’:
// t.c:1:27: warning: the address of ‘i’ will never be NULL [-Waddress]
 int f (int i) { return &i != (void *) 0; }
//                         ~~~^~~~~~~~~~~~~
// t.c: In function ‘int g(int)’:
// t.c:3:27: warning: the address of ‘i’ will never be NULL [-Waddress]
 int g (int i) { return &i != (int *) 0; }
//                         ~~~^~~~~~~~~~~~


