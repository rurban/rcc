/* GCC Bug #21038 - report unmatched brace as a note
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=21038
 */


void foo() {

    int i;
    int j;
    int k;
// you get:
// ~/ootbc/members/src$ g++ foo.cc
// foo.cc: In function `void foo()':

// The disgnostic should show the line number of the unmatched opener "{". Finding
// mismatched brackets can be a real pain in lengthy or nested code such as .h
// files for collection template classes.
// Ivan


