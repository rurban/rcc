/* GCC Bug #117197 - ICE: 'verify_gimple' failed with vector_size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117197
 */
/* { dg-do compile } */


struct b {
  __attribute__((vector_size(4))) int a;
};
int c;
void d() { struct b e = {0, 1, 2, 3}; }
//     5 | void d() { struct b e = {0, 1, 2, 3}; }
// t.c:5:29: note: (near initialization for ‘e’)
//     5 | void d() { struct b e = {0, 1, 2, 3}; }
// t.c:5:32: note: (near initialization for ‘e’)
//     5 | void d() { struct b e = {0, 1, 2, 3}; }
// t.c:5:35: note: (near initialization for ‘e’)
    struct b e = {.a={ 0 }};
//     5 | void d() { struct b e = {c, .a = 2}; }
//     5 | void d() { struct b e = {c, .a = 2}; }


