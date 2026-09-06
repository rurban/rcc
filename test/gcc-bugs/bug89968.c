/* GCC Bug #89968 - attribute packed fails to reduce char vector member alignment
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89968
 */
/* { dg-do compile } */


struct S
{
  char c;
  __attribute__ ((aligned (64), packed, vector_size (1024))) char v;
};

int f (void) { return sizeof (struct S); }
int g (void) { return __alignof__ (struct S); }
// GCC generates sizeof (struct S) == 2048 and __alignof__ (struct S) == 1024;
// expected sizeof == 1088 and __alignof__ == 64 (the vector_size/aligned/
// packed attributes should reduce alignment of the char vector member).

