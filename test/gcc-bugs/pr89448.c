/* GCC Bug #89448 - Failure to generate diagnostic for "complex int" (OK for "_Complex int")
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89448
 */
/* { dg-do compile } */


#include <complex.h>
int lower (void)
{
  complex int a = 0;
  return creal (a);
}
int upperunder (void)
{
  _Complex int a = 0;
  return creal (a);
}
int redef (void)
{
#define complex _Complex
  complex int a = 0;
  return creal (a);
}
   _Complex int a = 0;
 #define complex _Complex
   complex int a = 0;

// Why no diagnostic in lower() but yes in redef()?  Sounds as if the

// The preprocessed form just further confuses the matter, because (as expected) "complex" does appear to be just "_Complex":

int lower (void)
{

         int a = 0;
  return creal (a);
}


