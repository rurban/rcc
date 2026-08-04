/* GCC Bug #115684 - No warning for pointer and enum field comparison
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115684
 */


#include <stdio.h>

int main(void) {
    enum { myEnumField0, myEnumField1 };
    int a = 0;
    int *b = &a;
    if (b == myEnumField0)
        return puts("hey");
    else if (b == myEnumField1)
 return puts("yeh");
    return a;
}
// 115684.c: In function 'main':
// 115684.c:9:16: warning: comparison between pointer and integer
//     9 |     else if (b == myEnumField1)
//       |                ^~
// $
// So, it depends on the value of the enumerator, it seems. Also it's a problem that the existing warning doesn't seem to be linked to a flag; see <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - [meta-bug] Some warnings are not linked to diagnostics options"
//    href="show_bug.cgi?id=44209">bug 44209</a>


