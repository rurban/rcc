/* GCC Bug #96503 - attribute alloc_size effect lost after inlining
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96503
 */
/* { dg-do compile } */


__attribute__ ((alloc_size (1))) int* f1 (int n) { return f (n); }

void h1 (void)
{
  int *p = f1 (3);
  __builtin_memset (p, 0, 3 * sizeof p);   // missing warning
}

// where in the IR we should insert the call to the internal function  .ACCESS_WITH_SIZE (REF, COUNTED_BY_REF, (* TYPE_OF_SIZE)0, TYPE_SIZE_UNIT for element)? 
//  p = f1 (3)

// we will wrap the pointer "p" with .ACCESS_WITH_SIZE (p, 3, 0, 1), i.e, 
// .ACCESS_WITH_SIZE (p, 3, 0, 1) = f1 (3);


