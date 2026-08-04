/* GCC Bug #100670 - unused attribute ignored on function definition
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100670
 */


typedef void *void_ptr;

static TYPE __attribute__((unused))
// foo (void)
{
  return 0;
}
// ...
// Let's try with type int:
// ...
// $
// ...
// No warning.
// Ok, now with void *:
// ...
// foo.c:4:1: error: ‘foo’ defined but not used [-Werror=unused-function]
//     4 | foo (void)
//       | ^~~
// cc1: all warnings being treated as errors
// ...
// Hmm, a warning.
// Now with void_ptr:
// ...
// $
// ...
// again no warning.
// My expectation was that there would be no warning for the "void *" case.
// I cannot tell from the gcc attributes syntax documentation what is the correct behaviour.
// If this is not a compiler bug, then I suggest this is changed into a documentation PR.
// Note that there is at least one example that uses this style ( <a href="https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes">https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes</a> ):
// ...
void __attribute__ ((visibility ("protected")))
f () { /* Do something. */; }
// ...


