/* GCC Bug #80199 - Wlogical-op inconsistent from int to float ?
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80199
 */
/* { dg-do compile } */


void g1( float );
void g2( int );

// f1( float a)
{
	if (a < 0.0 && a > 1.0)
// 		g1( a);
}

// f2( int a)
{
	if (a < 0 && a > 1)
// 		g2( a);
}
// mar26e.cc: In function ‘void f2(int)’:
  if (a < 0 && a > 1)


