/* GCC Bug #67314 - Split warning for assigning an out-of-range integer to an enum out from -Wc++-compat into a separate flag, -Wassign-enum
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67314
 */


enum E {e} ee = 99;
// $: gcc-trunk -Wall -Wextra -c t.c
// $: clang-trunk  -c t.c
// $: cat t.c
enum E {e} ee = 99;
// $: gcc-trunk -Wall -Wextra -c t.c
// $: clang-trunk  -Wassign-enum -c t.c
// t.c:1:17: warning: integer constant not in range of enumerated type 'enum E' [-Wassign-enum]
enum E {e} ee = 99;
//                 ^
// 1 warning generated.


