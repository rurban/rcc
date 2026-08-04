/* GCC Bug #93573 - internal compiler error: in force_constant_size, at gimplify.c:733
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93573
 */
/* { dg-do compile } */


int f1 ( char * p ) ( ) { int x ;
 x = 4 ;
 if ( ! x != 10 ) return 1 ;
 if ( ( sum ( 1 , 2 ) / 2 ) != 1 ) return 1 ;
 if ( - ( 2 * sum ( 3 , 4 ) + sum ( ( union foo { int i , X [ 2 ] [ - ( 100.000000 / 2 ) * 2 ] , k ;
 char * p ;
 float ( * f1 ( int a , int b ) ) ( int c , int b ) ;
 } ) p , 2 ) ) != 0 - 4 ) return 1 ;
 return 0 ;
 }


