/* GCC Bug #80801 - Error "void value not ignored as it ought to be" is a bit cryptic
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80801
 */
/* { dg-do compile } */


void func()
{
}

int main()
{
  return func(); /* { dg-error "void value not ignored as it ought to be" } */
}
// void.cpp: In function 'int main()':
// void.cpp:7:15: error: void value not ignored as it ought to be
// return func();
//                ^
// In this small example it is clear, for complex code it might not be obvious what the error is.
// clang is a little bit more helpful:
// void.cpp:7:10: error: cannot initialize return object of type 'int' with an rvalue of type 'void'
// return func();
//          ^~~~~~
// 1 error generated.
// Best regards,
// Martin


