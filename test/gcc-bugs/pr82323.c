/* GCC Bug #82323 - circular ifunc attribute on a function definition silently accepted
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82323
 */
/* { dg-do compile } */


int g (s)
    char *s;
  {
    return __builtin_strlen (s);
  }


