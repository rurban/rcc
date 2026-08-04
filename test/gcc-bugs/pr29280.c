/* GCC Bug #29280 - misleading warning for assignment used as truth construct
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=29280
 */


int f (int a, int b)
{
    if (a = b)
        return 1;
    return 2;
}
// t.c:3:11: warning: using the result of an assignment as a condition without
//       parentheses [-Wparentheses]
    if (a = b)
//         ~~^~~
// t.c:3:11: note: place parentheses around the assignment to silence this warning
    if (a = b)
//           ^
//         (    )
// t.c:3:11: note: use '==' to turn this assignment into an equality comparison
    if (a = b)
//           ^
//           ==
// 1 warning generated.


