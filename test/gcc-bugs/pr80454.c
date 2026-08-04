/* GCC Bug #80454 - -Wmissing-braces wrongly warns about universal zero initializer {0}
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80454
 */


typedef union { char c; int i; } union_t;

struct { struct { int a; long b; } x; int y; } t = { { 0 }, 1 };

struct { struct { union_t a; long b; } x; int y; } u = { { 0 }, 1 };

// As expected, I do not get a warning for the first initialization. But with Debian's GCC 6.3.0 20170415 (and previous versions GCC 4.9 and 5), I get a warning for the second one:

// tst.c:5:56: warning: missing braces around initializer [-Wmissing-braces]
 struct { struct { union_t a; long b; } x; int y; } u = { { 0 }, 1 };
//                                                         ^
// tst.c:5:56: note: (near initialization for ‘u’)

// (pthread_rwlock_t from <pthread.h> is such a union type on my machine, so that there's the same problem with this.)
// It seems that not everything was fixed in <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
   title="RESOLVED FIXED - -Wmissing-braces wrongly warns about universal zero initializer {0}"
//    href="show_bug.cgi?id=53119">PR 53119</a>.


