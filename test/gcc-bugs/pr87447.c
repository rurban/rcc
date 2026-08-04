/* GCC Bug #87447 - Missing -Wconversion warning in implicit conversion of unsigned long long to double with comparison operators
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87447
 */
/* { dg-do compile } */


int foo (double x)
{
  return x == (1ULL << 63) + 1;
}

// Here, (1ULL << 63) + 1 is implicitly converted to double, thus with a change of value (since double has only a 53-bit significand and (1ULL << 63) + 1 needs 64 bits). Therefore, a -Wconversion warning is expected.
// Tested with:


