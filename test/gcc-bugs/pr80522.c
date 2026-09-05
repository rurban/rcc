/* GCC Bug #80522 - Enhancement request: __attribute__((warn_untested_result))
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80522
 */
/* { dg-do compile } */


// Enhancement request: __attribute__((warn_untested_result)), an attribute
// similar to __attribute__((warn_unused_result)) but for things like
// allocation failures that are not verified before use. For instance:
//     void *malloc(size_t size);
// could become
//     void * __attribute((warn_untested_result)) malloc(size_t size)
// so that
#include <stdlib.h>

    struct foo {
            int bar;
    };

    struct foo *alloc_foo(void)
    {
            struct foo *baz = malloc(sizeof(struct foo));
//             baz->bar = 1;
            return baz;
    }
// The compiler could emit a warning on the set
// of baz->bar as an intermediate test of baz
// is not performed before any use of baz.
// Martin Sebor also mentioned that non-allocation
// functions like fopen could also use this __attribute__
// mechanism.


