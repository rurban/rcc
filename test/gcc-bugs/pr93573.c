/* GCC Bug #93573 - internal compiler error: in force_constant_size, at gimplify.c:733
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93573
 * NOTE: this exact POC's ICE (error-recovery treating an invalid array as
 * VLA) was fixed for GCC 10 (r10-7438); it now just reports the errors
 * below without crashing. A separate, still-open regression (union cast to
 * a VLA union type, e.g. "union C { int d[b]; char *e; }; bar((union C)&a);"
 * from comment #3/#4) continues to ICE on some versions but needs a
 * language-standard-sensitive prototype not reproducible as cleanly here.
 */
/* { dg-do compile } */


int f1 ( char * p ) ( ) { int x ; /* { dg-error "declared as function returning a function" } */
 x = 4 ;
 if ( ! x != 10 ) return 1 ;
 if ( ( sum ( 1 , 2 ) / 2 ) != 1 ) return 1 ; /* { dg-error "implicit declaration of function" } */
 if ( - ( 2 * sum ( 3 , 4 ) + sum ( ( union foo { int i , X [ 2 ] [ - ( 100.000000 / 2 ) * 2 ] , k ; /* { dg-error "size of array .X. has non-integer type" } */
 char * p ;
 float ( * f1 ( int a , int b ) ) ( int c , int b ) ; /* { dg-error "declared as a function" } */
 } ) p , 2 ) ) != 0 - 4 ) return 1 ;
 return 0 ;
 }


