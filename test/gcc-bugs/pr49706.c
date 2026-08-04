/* GCC Bug #49706 - No warning for (!x > 1) which is always false
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=49706
 */
/* { dg-do compile } */
/* { dg-options "-Wextra -Wall" } */

void warn (void);

int
f (int a)
{
  if (!a > 1) /* { dg-warning "logical not" } */
    warn ();
  return 0;
}

// Comment 2 notes GCC also fails to warn for similar always-false
// comparisons of an inherently 0/1-valued expression against something
// outside that range, e.g. (still unimplemented as of this writing):
//
// _Bool x;
// ...
// if (x > 1)
// or
// struct { unsigned int i : 1; } x;
// ...
// if (x.i > 1)
// we don't warn either.
