/* GCC Bug #101446 - -Wpedantic causes an error with zero size array
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101446
 */
/* { dg-do compile } */
/* { dg-options "-pedantic-errors" } */

static const char __gbk_from_ucs4_tab9[][2]
  = {}; /* { dg-error "ISO C forbids empty initializer braces" } */
