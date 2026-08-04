/* GCC Bug #80959 - -Wreturn-type "control reaches end of non-void function" false positive with -fsanitize=address
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80959
 */
/* { dg-do compile } */


{
// ...
    }
//   finally
    {
      ASAN_MARK (POISON, &n, 4);
    }
// which eventually gets to
//   <bb 5> [0.00%]:
  ASAN_MARK (POISON, &n, 4);
  switch (finally_tmp.2) <default: <L8> [0.00%], case 1: <L5> [0.00%]>

// <L5> [0.00%]:

//   <bb 7> [0.00%]:
//   return;

// <L8> [0.00%]:
  return D.2131;
// thus it lacks a return.  That's also visible in .original:
{
  int n;

    int n;
  bar (&n);
  switch (i)
    {
      case 1:;
      switch (i)
        {
          default:;
          return 0;
        }
      goto <D.2128>;
      default:;
      return 0;
    }
//   <D.2128>:;
}

// but I guess that "dead" break; after the inner switch was previously
// CFG cleaned-up.


