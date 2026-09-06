/* GCC Bug #65672 - type attribute "aligned" can decrease alignment
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65672
 */
/* { dg-do compile } */


typedef float vec __attribute__((vector_size(16)));
typedef float vecu __attribute__((vector_size(16),aligned(8)));

// alignof(vec) says 16 and alignof(vecu) says 8. And indeed,

vec f(vecu*p){
  return *p;
}

// compiles to movups (while regular loads use movaps), so this behavior is useful. Assuming it is meant to work and not just an accident, it would be nice to document this (otherwise it is a wrong code bug).


