/* GCC Bug #70730 - Inconsistent column number in "error: attempt to take address of bit-field structure member"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70730
 */


#include <stddef.h>

int main()
{
  struct s { int x:2; };
  typedef struct s s;

  offsetof(struct { int x:2; }, x);
//   offsetof(struct s, x);
//   offsetof(s, x);
}
   offsetof(struct { int x:2; }, x);
//    offsetof(struct s, x);
//    offsetof(s, x);
  offsetof(struct { int x:2; }, x);
#define offsetof(t, d) __builtin_offsetof(t, d)
  offsetof(struct { int x:2; }, x);
//   offsetof(struct s, x);
#define offsetof(t, d) __builtin_offsetof(t, d)
  struct s { int x:2; };
//   offsetof(s, x);
#define offsetof(t, d) __builtin_offsetof(t, d)
  struct s { int x:2; };


