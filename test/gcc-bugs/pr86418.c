/* GCC Bug #86418 - warn about mismatch in type between argument and parameter type for declaration without prototype
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86418
 */


extern void abort (void);
static void bar ();
// int
// main (void)
{
  bar (1);
  return 0;
}

static void
// bar (double i)
{
  if (i)
    abort ();
}
// ...
// The test-case contains undefined behaviour:
// - bar is declared without prototype
// - the call to bar in main is before the definition of bar, so the call is
//   considered calling a function that does not include a prototype (as opposed
//   to the situation where we move the definition of bar to before main, and the
//   call uses the prototype provided by the definition of bar)
// - the int argument in the call is incompatible with the double parameter of the
//   definition of bar
// In words of the c standard:
// ...
//  [ 6.9.1 Function definitions ]
//  If the declarator includes a parameter type list, the list also specifies the
 types of all the parameters; such a declarator also serves as a function
//  prototype for later calls to the same function in the same translation unit. 
// ...
and:
// ...
//  [ 6.5.2.2 Function calls ]
// If the expression that denotes the called function has a type that does not include a prototype, the integer promotions are performed on each argument, and arguments that have type float are promoted to double. These are called the default argument promotions. If the number of arguments does not equal the number of parameters, the behavior is undefined. If the function is defined with a type that includes a prototype, and either the prototype ends with an ellipsis (, ...) or the types of the arguments after promotion are not compatible with the types of the parameters, the behavior is undefined.
// ...
and:
// ...
// [ J.2 Undefined behavior ]
// For call to a function without a function prototype in scope where the function is defined with a function prototype, either the prototype ends with an ellipsis or the types of the arguments after promotion are not compatible with the types of the parameters (6.5.2.2).
// ...
// Atm, the only warning we get is when cloning:
// ...
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - [4.6 Regression] ICE: verify_stmts failed: conversion of register to a different size with -fno-early-inlining"
//    href="show_bug.cgi?id=48063">pr48063</a>.c  -Wall
// <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - [4.6 Regression] ICE: verify_stmts failed: conversion of register to a different size with -fno-early-inlining"
//    href="show_bug.cgi?id=48063">pr48063</a>.c: In function ‘bar.constprop’:
// <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - [4.6 Regression] ICE: verify_stmts failed: conversion of register to a different size with -fno-early-inlining"
//    href="show_bug.cgi?id=48063">pr48063</a>.c:18:6: warning: ‘i’ is used uninitialized in this function [-Wuninitialized]
   if (i)
//       ^
// ...
// The cloned version has no parameters, and replaces parameter i with local variable i, which is uninitialized:
// ...
// bar.constprop ()
{
  double i;
//   <bb 2>
  if (i_1(D) != 0.0)
    goto <bb 3>;
//   else
    goto <bb 4>;
//   <bb 3>
  abort ();
//   <bb 4>
//   return;
}
// ...


