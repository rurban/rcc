/* GCC Bug #69480 - Bad error message on assigning to read-only
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69480
 */
/* { dg-do compile } */


#include <stdint.h>

struct fail {
        uint32_t *p;
};

#define __ACCESS_ONCE(x) ({ \
         typeof(x) __var = ( typeof(x)) 0; \
                         (volatile const typeof(x) *)&(x); })
#define ACCESS_ONCE(x) (*__ACCESS_ONCE(x))

void fail(struct fail *s, uint32_t p) {
        ACCESS_ONCE(*s->p) = p; /* { dg-error "assignment of read-only location" } */
}


