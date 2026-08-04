/* GCC Bug #44515 - improve message for missing ";"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44515
 */
/* { dg-do compile } */


#include that previously gave inscrutable output, and improving e.g.:

  int i
  int j;
from:
//   t.c:2:1: error: expected '=', ',', ';', 'asm' or '__attribute__' before 'int'
   int j;
//    ^~~
to:
//   t.c:1:6: error: expected ';' before 'int'
   int i
//         ^
        ;
   int j;
//    ~~~
// gcc.dg/noncompile/920923-1.c needs a slight update, as the output for
// the first line changes from:
//   920923-1.c:2:14: error: expected '=', ',', ';', 'asm' or
//   '__attribute__' before 'unsigned'
   typedef BYTE unsigned char; /* { dg-error "expected" } */
//                 ^~~~~~~~
to:
//   920923-1.c:2:13: error: expected ';' before 'unsigned'
   typedef BYTE unsigned char; /* { dg-error "expected" } */
//                ^~~~~~~~~
               ;
//   920923-1.c:2:1: warning: useless type name in empty declaration
   typedef BYTE unsigned char; /* { dg-error "expected" } */
//    ^~~~~~~
// The patch also adds a test for <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - improve message for missing ";""
//    href="show_bug.cgi?id=44515">PR c/44515</a> as a baseline.
// gcc/c/ChangeLog:
// 	<a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - syntax errors immediately before an #include are reported for within the included file"
//    href="show_bug.cgi?id=7356">PR c/7356</a>
// 	* c-parser.c (c_parser_declaration_or_fndef): Detect missing
// 	semicolons.
// gcc/testsuite/ChangeLog:
// 	<a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - syntax errors immediately before an #include are reported for within the included file"
//    href="show_bug.cgi?id=7356">PR c/7356</a>
// 	<a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - improve message for missing ";""
//    href="show_bug.cgi?id=44515">PR c/44515</a>
// 	* c-c++-common/<a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - improve message for missing ";""
//    href="show_bug.cgi?id=44515">pr44515</a>.c: New test case.
// 	* gcc.dg/<a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - syntax errors immediately before an #include are reported for within the included file"
//    href="show_bug.cgi?id=7356">pr7356</a>-2.c: New test case.
// 	* gcc.dg/<a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - syntax errors immediately before an #include are reported for within the included file"
//    href="show_bug.cgi?id=7356">pr7356</a>.c: New test case.
// 	* gcc.dg/spellcheck-typenames.c: Update the "singed" char "TODO"
 case to reflect changes to output.
// 	* gcc.dg/noncompile/920923-1.c: Add dg-warning to reflect changes
// 	to output.
Added:
//     trunk/gcc/testsuite/c-c++-common/<a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - improve message for missing ";""
//    href="show_bug.cgi?id=44515">pr44515</a>.c
//     trunk/gcc/testsuite/gcc.dg/<a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - syntax errors immediately before an #include are reported for within the included file"
//    href="show_bug.cgi?id=7356">pr7356</a>-2.c
//     trunk/gcc/testsuite/gcc.dg/<a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - syntax errors immediately before an #include are reported for within the included file"
//    href="show_bug.cgi?id=7356">pr7356</a>.c
Modified:
//     trunk/gcc/c/ChangeLog
//     trunk/gcc/c/c-parser.c
//     trunk/gcc/testsuite/ChangeLog
//     trunk/gcc/testsuite/gcc.dg/noncompile/920923-1.c
//     trunk/gcc/testsuite/gcc.dg/spellcheck-typenames.c


