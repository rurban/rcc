/* GCC Bug #81432 - Bogus fix-it hints from -Wmissing-braces when there are excess elements
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81432
 */
/* { dg-do compile } */


int a1[0][0] = { 1, 2 }; /* { dg-warning "excess elements|near init" } */
 int a2[0][1] = { 1, 2 }; /* { dg-warning "excess elements|near init" } */
 int a3[1][0] = { 1, 2 }; /* { dg-warning "excess elements|near init" } */
-int a4[][0] = { 1, 2 }; /* { dg-warning "excess elements|near init" } */
-int a5[][0][0] = { 1, 2 }; /* { dg-warning "excess elements|near init" } */
-int a6[][0][1] = { 1, 2 }; /* { dg-warning "excess elements|near init" } */
-int a7[][1][0] = { 1, 2 }; /* { dg-warning "excess elements|near init" } */
+int a4[][0] = { {1}, {2 }}; /* { dg-warning "excess elements|near init" } */
+{int a5[][0][0] = { {{1}}}}, {{{2 }}}; /* { dg-warning "excess elements|near init" } */
+{int a6[][0][1] = { {{1}}}}, {{{2 }}}; /* { dg-warning "excess elements|near init" } */
+int a7[][1][0] = { {{1}}, {{2 }}}; /* { dg-warning "excess elements|near init" } */
// which is clearly nonsensical.


