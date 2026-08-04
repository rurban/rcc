/* GCC Bug #115027 - Missing warning: unused struct's with self-referential initialisers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115027
 */


#include <stdio.h>

struct foo {
  struct foo *a;
};

static struct foo f = { &f };
int main()
{
//   printf("Hello\n");
}
// Here 'f' is unused outside of it's initialiser, pointing to itself.
// So technically used, but practically not.
// In the Linux kernel, this corresponds to it's LIST_HEAD :
// (from include/linux/list.h)

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
 struct list_head name = LIST_HEAD_INIT(name)
// and I've just gone through and posted patches to remove a handful of LIST_HEADs, some of which had been unused for many years.
// It would be nice if the compiler told people instead.


