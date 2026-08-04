/* GCC Bug #70186 - RFE: better handling of misspelled attributes
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70186
 */
/* { dg-do compile } */


struct S *foo __attribute__ ((visbility("hidden")));
//          ^
// Note the misspelling of the attribute name:
//   "visbility"
// should have read:
//   "visibility"
// The reported location of the diagnostic is also suboptimal; it should underline the attribute, either:

  struct S *foo __attribute__ ((visbility("hidden")));
//                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
or:
  struct S *foo __attribute__ ((visbility("hidden")));
//                                 ^~~~~~~~~~~~~~~~~~~
// Ideally we should use Levenshtein and emit a hint and a fixit:
//   t.c:3:8: warning: ‘visbility’ attribute directive ignored; did you mean 'visibility'? [-Wattributes]
  struct S *foo __attribute__ ((visbility("hidden")));
//                                 ^~~~~~~~~~~~~~~~~~~
//                                 ---------
//                                 visibility
// or somesuch.


