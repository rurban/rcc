/* GCC Bug #67729 - -Wformat should warn for %Ns where the buffer size is known to be less than N in size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67729
 */
/* { dg-do compile } */

#include <stdio.h>
extern void g( FILE * fp);

void f( FILE * fp)
{
 char buf[ 10];

	while (fscanf( fp, "%10s", buf))
		;
	while (fscanf( fp, "%5s", buf))
		;
	while (fscanf( fp, "%20s", buf))
		;
	g( fp);
}


