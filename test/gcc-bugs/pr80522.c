/* GCC Bug #80522 - Enhancement request: __attribute__((warn_untested_result))
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80522
 */


__attribute__((warn_unused_result))
// might be
__attribute__((warn_untested_result))

for things like allocation failures that
// are not verified before use.
// For instance:
    void *malloc(size_t size);
// could become
    void * __attribute((warn_untested_result)) malloc(size_t size)
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
    struct foo *alloc_foo(void)
    {
            struct foo *baz = malloc(sizeof(struct foo));
            if (baz) baz->bar = 1;
            return baz;
    }
// This variant would not emit a warning.
// Similarly, alloc_foo could use that new attribute.
// Martin Sebor also mentioned that non-allocation
// functions like fopen could also use this __attribute__
// mechanism.


