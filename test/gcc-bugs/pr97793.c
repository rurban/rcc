/* GCC Bug #97793 - Bogus return-type warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97793
 */


static int
// foo ()
{
  for (;;)
    ;
}

int (*x)() = foo;


