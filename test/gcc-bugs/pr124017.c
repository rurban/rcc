/* GCC Bug #124017 - Incomplete/Misleading diagnostic locations for the same undefined label in multiple goto statements
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124017
 */
/* { dg-do compile } */


// /* test_goto.c */
void test_function(int logic_gate) {
    if (logic_gate > 10) {
        goto lbl_missing;  /* Violation 1: Line 4 */
    }

    int x = 0;
    if (logic_gate < 5) {
        goto lbl_missing;  /* Violation 2: Line 9 */
    }

//     x++;
    goto lbl_missing;      /* Violation 3: Line 13 */
}
// Steps to reproduce: gcc -c test_goto.c
// Actual Results:
// test_goto.c: In function 'test_function':
// test_goto.c:13:5: error: label 'lbl_missing' used but not defined
//    13 |     goto lbl_missing;
//       |     ^~~~
// (Note: Lines 4 and 9 are completely ignored in the error report.)

// While GCC does produce a diagnostic, its failure to identify the earlier goto statements as constraint violations at their respective coordinates makes the diagnostic information incomplete. In large, machine-generated, or complex files (like the attached case), this behavior forces a "trial-and-error" debugging process where the developer fixes one line only to have the compiler "discover" the previous one in a subsequent pass.
// The complete large file is attached. The same error appears at lines 772, 808, and 923, yet GCC reports only an error at line 923.


