/* GCC Bug #49626 - -Wsign-conversion ignores explicit conversion when followed by conversion to a wider type
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=49626
 */


unsigned long f()
{
  short sh;
  return (unsigned int)sh;
}


