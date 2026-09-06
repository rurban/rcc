/* GCC Bug #120511 - Initializer warning on universal zero initializer for array element with double member
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=120511
 */
/* { dg-do compile } */


struct test {
                double val;
                int flag;
        };

        struct test test[] = {
                {.val = 1,      .flag = 0},
                {.val = 2,      .flag = 1},
                {0}
        };
// The warning is not generated if I try the above code with the member val declared as an int instead of a double.  Likewise it does not occur if I use a single struct instead of an array.  It seems that a combination of an initial floating-point type member, and trying to universal-zero-initialize one of the array elements, triggers this erroneous warning.
// Demonstrated with GCC version 15.1.0, built from released source tarball on a Debian system.  This also happens on GCC 12 and 13 for Debian/Ubuntu.
// Command: "gcc-15 -W gcc-init-bug.c -c".  Output below:
//         gcc-init-bug.c:9:9: warning: missing initializer for field ‘flag’ of ‘struct test’ [-Wmissing-field-initializers]
//             9 |         {0}
//               |         ^
//         gcc-init-bug.c:3:13: note: ‘flag’ declared here
//             3 |         int flag;
//               |             ^~~~


