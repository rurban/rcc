/* GCC Bug #99955 - gcc.c-torture/execute/pr92618.c violates strict aliasing rules
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99955
 */
/* { dg-do compile } */


typedef long long __m128i __attribute__((__may_alias__, __vector_size__(2 * sizeof (long long))));

// (but vector type building is a bit iffy).
typedef long long __m128i __attribute__((__vector_size__(2 * sizeof (long long)), __may_alias__));


