/* GCC Bug #87779 - Extremely large expression causes segfault
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87779
 */


#include<stdio.h>

int main(void) {
    int x = !!![one megabyte of exclamation marks]!!!1;
//     printf("%d", x);
    return 0;
}
// ----
// The same error occurs for the corresponding g++ command.
// (I haven't come across this problem in the real world - I was just playing around.)


