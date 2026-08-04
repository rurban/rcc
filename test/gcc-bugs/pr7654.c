/* GCC Bug #7654 - warn if an enum is being assigned a non enum value
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=7654
 */


enum e1 { e1a, e1b };
enum e1 e1v;
enum e2 { e2a, e2b };
enum e2 e2v;
int i;
e1v = 1; // warning
e1v = e1a; // ok
e2v = e1v; // warning
// i = e1v; // ok I guess
e2v = i; // warning


