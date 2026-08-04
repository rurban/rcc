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
        ACCESS_ONCE(*s->p) = p;
}
// ====
// results in
// fail.c: In function 'fail':
// fail.c:13:21: error: assignment of read-only location '*__builtin_memcpy(&<U1950>, &({...}))'
  ACCESS_ONCE(*s->p) = p;
//                      ^
// There is no __builtin_memcpy in the program source, but the <U1950> is
// even more worrying (the number is different on different runs).


