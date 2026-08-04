/* GCC Bug #82354 - semi-colon instead of comma in parameter list produces confusing diagnostics
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82354
 */


int main(int argc; char **argv) { (void) argc; (void) argv; return 0; }
// With -Wpedantic this prints:
// sscce.c:1:1: warning: ISO C forbids forward parameter declarations [-Wpedantic]
 int main(int argc; char **argv) { (void) argc; (void) argv; return 0; }
//  ^~~
// sscce.c:1:14: error: parameter ‘argc’ has just a forward declaration
 int main(int argc; char **argv) { (void) argc; (void) argv; return 0; }
//               ^~~~
// sscce.c:1:5: warning: first argument of ‘main’ should be ‘int’ [-Wmain]
 int main(int argc; char **argv) { (void) argc; (void) argv; return 0; }
//      ^~~~
// sscce.c:1:5: warning: ‘main’ takes only zero or two arguments [-Wmain]
// sscce.c: In function ‘main’:
// sscce.c:1:42: error: ‘argc’ undeclared (first use in this function); did you mean ‘argv’?
 int main(int argc; char **argv) { (void) argc; (void) argv; return 0; }
//                                           ^~~~
//                                           argv
// sscce.c:1:42: note: each undeclared identifier is reported only once for each function it appears in
// Without -Wpedantic it's even less clear what's going on.
// Most users have probably got no idea what "forward parameter declarations" are (is this extension even documented?) so maybe it would be better to assume the user didn't mean to type a semi-colon there.

// Instead of a cryptic message about an obscure extension, maybe suggest a fix-it to replace the ';' with ','


