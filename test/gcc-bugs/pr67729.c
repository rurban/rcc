/* GCC Bug #67729 - -Wformat should warn for %Ns where the buffer size is known to be less than N in size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67729
 */


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
// 	g( fp);
}
// Here is cppcheck detecting the problem and suggesting a fix.
// Checking sep9a.cc...
// [sep9a.cc:12]: (error) Width 10 given in format string (no. 1) is larger than destination buffer 'buf[10]', use %9s to prevent overflowing it.
// [sep9a.cc:16]: (error) Width 20 given in format string (no. 1) is larger than destination buffer 'buf[10]', use %9s to prevent overflowing it.
// $


