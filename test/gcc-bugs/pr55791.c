/* GCC Bug #55791 - gcc fails to detect wrong type in sizeof in malloc
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55791
 */


extern void g(char *);

void f()
{
 char *p;

// 	// wrong type in sizeof in malloc
 p = (char *) malloc( 10 * sizeof( char *));

// 	g(p);
}
 p = (char *) malloc( 10 * sizeof( char));

// i.e. the type in the sizeof expression must be the same as (or 
// at very least the same size as) the type of destination buffer.


