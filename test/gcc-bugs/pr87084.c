/* GCC Bug #87084 - Excessive diagnostic messages for invalid use of __builtin_va_arg_pack{,_len}() in a loop
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87084
 */
/* { dg-do compile } */
/* { dg-options "-O3" } */

int xc;

void
zp (void)
{
  int ta;

  for (ta = 0; ta < 8; ++ta)
    {
      int ij;

      for (ij = 0; ij < 17; ++ij)
        xc = __builtin_va_arg_pack ();
    }
}
