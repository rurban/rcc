/* GCC Bug #108753 - '-Wduplicated-cond' doesn't diagnose duplicated subexpressions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108753
 */


if (a == 5) // { dg-note {previously used here} }
      return 30;
    else if (a == 5) // { dg-warning {duplicated 'if' condition} }
      return 40;
// ..., but this and similar ones don't:
    if (a == 5) // { dg-note {previously used here} TODO { xfail *-*-* } }
      return 30;
    else if (a == 5 // { dg-warning {duplicated 'if' condition} TODO { xfail *-*-* } }
//              || a == 6)
      return 40;


