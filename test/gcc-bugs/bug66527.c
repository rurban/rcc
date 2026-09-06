/* GCC Bug #66527 - incorrect line number in diagnostics for multiline initializers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66527
 */
/* { dg-do compile } */


/* 1 */ void f (register int i) {
// /* 2 */ int* a =
// /* 3 */     &i
// /* 4 */ ;
/* 5 */ }
// t.c: In function ‘f’:
// t.c:4:9: error: address of register variable ‘i’ requested
//  /* 4 */ ;
//          ^
// (I haven't seen this mentioned on the <a href="https://gcc.gnu.org/wiki/Better_Diagnostics">https://gcc.gnu.org/wiki/Better_Diagnostics</a> Wiki; sorry if this is another known issue and I missed it.)


