/* GCC Bug #87588 - gcc does not warn about unused variable which references to itself
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87588
 */
/* { dg-do compile } */


struct list {
        struct list *prev, *next;
};

struct mutex {
        struct list waiters;
};

static struct mutex mutex = \
        {.waiters = {.prev = &mutex.waiters, .next = &mutex.waiters}};

static int a;
static int *b = &a;
// ---->8-----
// produces:
//   a.c:14:13: warning: 'b' defined but not used [-Wunused-variable]
//   14 | static int *b = &a;
//      |             ^
// Would it be possible to also warn that the variable 'mutex' is unused? Except that its member (.list.prev) is referenced to itself, it is not used outside of that "initialization" construct.


