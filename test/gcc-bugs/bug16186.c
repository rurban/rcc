/* GCC Bug #16186 - gcc should have an option to warn about enumerations with duplicate values
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=16186
 */
/* { dg-do compile } */


enum status { ok = 0, err = -1, foo, bar };
//                                 ^~~
// j.c:1:15: note: element 'ok' also has value 0
enum status { ok = 0, err = -1, foo, bar };
//               ^~~~~~
// I suppose we could do a binary search when adding a new enumerator in build_enumerator, see if an enumerator with the same value is already present, and warn if so.


