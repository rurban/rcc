/* GCC Bug #116357 - The item's address of the array is not correct if aligned is used
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116357
 */
/* { dg-do compile } */

/* Arrays of over-aligned element types are invalid C (an element type
 * with TYPE_ALIGN > TYPE_SIZE breaks ARRAY_REF).  Comment 8's case: the
 * C frontend failed to reject the volatile variant and miscompiled the
 * array at -O1+ (all elements got the same address).  Fixed by Jakub's
 * gcc15-pr116357.patch (comment 10-11) - gcc now diagnoses it. */
typedef volatile int int64 __attribute__((aligned (8)));
int64 a[4]; /* { dg-error "alignment of array elements is greater than element size" } */

typedef double adouble __attribute__((aligned(8)));
struct X
{
  int n;
  adouble x[1024]; /* valid: double is naturally 8-aligned */
};