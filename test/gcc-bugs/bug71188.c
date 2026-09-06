/* GCC Bug #71188 - missing warning converting constant integer expression zero to pointer
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71188
 */
/* { dg-do compile } */


struct S { int i; };

int a [0];

int *p = __builtin_offsetof (struct S, i);
int *q = sizeof a;
int *r = (char*)&((struct S*)0)->i - (char*)((struct S*)0);
// u.cpp:3:5: warning: ISO C forbids zero-size array ‘a’ [-Wpedantic]
 int a [0];
//      ^
// u.cpp:7:10: warning: initialization makes pointer from integer without a cast [-Wint-conversion]
 int *r = (char*)&((struct S*)0)->i - (char*)((struct S*)0);
//           ^


