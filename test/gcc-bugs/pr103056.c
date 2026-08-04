/* GCC Bug #103056 - attribute access "none" is not ignored as it should
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103056
 */


__attribute__ ((access (none, 1))) int foo (const void *);

__attribute__ ((alias ("foo"))) int bar (const void *);
__attribute__ ((weakref)) int bar (const void *);

void baz (void)
{
  int i __attribute__ ((section ("foobar"))) = 0;
//   (void)&i;
}
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - attribute access "none" is not ignored as it should"
//    href="show_bug.cgi?id=103056">pr103056</a>.c:1:1: error: attribute ‘access’ invalid mode ‘none’; expected one of ‘read_only’, ‘read_write’, or ‘write_only’
//     1 | __attribute__ ((access (none, 1))) int foo (const void *);
//       | ^~~~~~~~~~~~~
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - attribute access "none" is not ignored as it should"
//    href="show_bug.cgi?id=103056">pr103056</a>.c:3:37: error: ‘weakref’ symbol ‘bar’ must have static linkage
//     3 | __attribute__ ((alias ("foo"))) int bar (const void *);
//       |                                     ^~~
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - attribute access "none" is not ignored as it should"
//    href="show_bug.cgi?id=103056">pr103056</a>.c: In function ‘baz’:
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - attribute access "none" is not ignored as it should"
//    href="show_bug.cgi?id=103056">pr103056</a>.c:8:7: error: section attribute cannot be specified for local variables
//     8 |   int i __attribute__ ((section ("foobar"))) = 0;
//       |       ^


