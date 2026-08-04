/* GCC Bug #43778 - C/C++ __attribute__((deprecated)) does not appear to wrap declarations as implied from the doc.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=43778
 */


#define Deprecated __attribute__((deprecated))

typedef int INT Deprecated;   // no warning (ok)

typedef INT FOO Deprecated;   // no warning (ok)

struct Deprecated S {
  INT i;                      // warning (bug?)
};

INT f1 (void) Deprecated;     // no warning (ok)

void f2 (INT) Deprecated;     // warning (bug)

INT f3 (INT) Deprecated;      // warning (bug)

INT i Deprecated;             // no warning (ok)
// t.C:8:3: warning: ‘INT’ is deprecated [-Wdeprecated-declarations]
   INT i;                      // warning (bug?)
//    ^~~
// t.C:13:1: warning: ‘INT’ is deprecated [-Wdeprecated-declarations]
 void f2 (INT) Deprecated;     // warning (bug)
//  ^~~~
// t.C:15:1: warning: ‘INT’ is deprecated [-Wdeprecated-declarations]
 INT f3 (INT) Deprecated;      // warning (bug)
//  ^~~
// G++ comes closer to the expected output but it fails the typedef test (which GCC passes in C mode):

// t.C:8:7: warning: ‘INT’ is deprecated [-Wdeprecated-declarations]
   INT i;                      // warning (bug?)
//        ^
// t.C:3:13: note: declared here
 typedef int INT Deprecated;   // no warning (ok)
//              ^~~
// Clang 5.0 produces no warnings.


