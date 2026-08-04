/* GCC Bug #100529 - ICE at -O3: in force_constant_size, at gimplify.c:733 since r11-4494-ga4223abb3deb24e8
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100529
 */
/* { dg-do compile } */

int map();
void bar(...);
void foo(char a) {
  union C {
    int d[map()];
    char *e;
  };
  bar((union C) & a);
}

