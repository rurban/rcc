/* GCC Bug #57021 - Better error message for * missing an expression or rather using a non-type as a cast
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57021
 */


int main(void)
{
  int a;
//   (a*)0;
}
// ---- CUT ---
// Right now we produce:
// t.c:4:6: error: expected expression before ‘)’ token
//    (a*)0;
//       ^
// or for C++:
// t.c:4:6: error: expected primary-expression before ‘)’ token
//    (a*)0;
//       ^
// t.c:4:7: error: expected ‘;’ before numeric constant
//    (a*)0;
//        ^
// ---- CUT ----
// Really we should say instead a is not a type.  Yes this is a complex error recovery issue but I think we should be able to figure it out.


