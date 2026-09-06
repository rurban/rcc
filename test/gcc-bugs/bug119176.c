/* GCC Bug #119176 - invalid use of `#pragma GCC novector` outside of a function causes an ICE
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119176
 */
/* { dg-do compile } */


// 29 |   _Pragma("GCC novector") for (int i = 0; i < DIST * 2; ++i) {                 \
//    29 |   _Pragma("GCC novector") for (int i = 0; i < DIST * 2; ++i) {                 \
//    29 |   _Pragma("GCC novector") for (int i = 0; i < DIST * 2; ++i) {                 \
//    29 |   _Pragma("GCC novector") for (int i = 0; i < DIST * 2; ++i) {                 \


