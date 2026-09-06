/* GCC Bug #122050 - statement expressions with an empty expression at the end have different behavior between C and C++
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=122050
 */
/* { dg-do compile } */


int foo() {
    return ({1; 2; 3;;});
}
// ```
// The C and C++ front-end have different behaviors here.
// From my anlysis of the differences in the front-end behavior:
// Looks like what is happening for the GCC's C front-end is an empty statement that is ; by itself does NOT add to the statement lists so it is ignored. This is the behavior since at least GCC 4.0. (I don't have any earlier to test). and then when finished with the statement expression, the front-end only looks at the statement list.
// While the GCC's C++ front-end does the processing/parsing of a statement expression differently. the end of the statement expression is handled while parsing of the last expression.
// Now the documentation of statement expressions (<a href="https://gcc.gnu.org/onlinedocs/gcc-15.2.0/gcc/Statement-Exprs.html">https://gcc.gnu.org/onlinedocs/gcc-15.2.0/gcc/Statement-Exprs.html</a>) seems to imply the C++ behavior though:
// ```
// The last thing in the compound statement should be an expression followed by a semicolon; the value of this subexpression serves as the value of the entire construct. (If you use some other kind of statement last within the braces, the construct has type void, and thus effectively no value.)
// ```
// Which one is right or is this just a documentation issue?


