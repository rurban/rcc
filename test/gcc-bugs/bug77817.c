/* GCC Bug #77817 - -Wimplicit-fallthrough: cpp directive renders FALLTHRU comment ineffective
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77817
 */
/* { dg-do compile } */
/* { dg-options "-Wimplicit-fallthrough" } */

// Testcase from PR77955, marked as a duplicate of this bug (comment #10):
// the FALLTHRU-style comment inside the braced case 1 block does not
// suppress the fallthrough warning (bogus warning), while the fallthrough
// from case 2 into default (intentionally not warned about, since it is a
// fallthru into an empty "break;") is correctly silent.

void bar(int);

void foo(int i) {
  switch (i) {
  case 1: {
    bar(1); /* { dg-warning "this statement may fall through" } */
    // fall-through
  }
  case 2:
    bar(2);
  default:
    break;
  }
}
