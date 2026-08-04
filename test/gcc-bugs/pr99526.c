/* GCC Bug #99526 - Casts should retain typedef information
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99526
 */


typedef int i;
  typedef const int cint;
  typedef const i ci;
  char v;
  typeof((i)v)        => int;
  typeof((cint)v)     => int;
  typeof((ci)v)       => int;
//   typeof((const i)v)  => int;
  typedef int i;
  typedef const int cint;
  typedef const i ci;
  char v;
  typeof((i)v)        => i;
  typeof((cint)v)     => int;
  typeof((ci)v)       => i;
//   typeof((const i)v)  => i;
// - systems (on POSIX: pid_t, uid_t, gid_t, ...) have some types that are of "unspecified" size, and it is helpful to be able to warn when these are intermixed.  Especially in printf() due to varargs calling:  there is no format specifier that is correct for these types.


