/* GCC Bug #65452 - strcmp (foo, foo) could give a warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65452
 */
/* { dg-do compile } */


#include <string.h>

struct S {
   int val;
};
int
f (struct S *sym1, struct S *sym2 __attribute__((unused)))
{
  return memcmp (&sym1->val, &sym1->val, sizeof (sym1->val));
}

#define N 0
#define M 0
int
f2 (void)
{
  extern const char *a[];
  return __builtin_strcmp (a[N], a[M]);
}


