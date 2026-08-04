/* GCC Bug #69415 - -Wmisleading-indentation warns on "if (__b < __a) return __b; return __a;"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69415
 */
/* { dg-do compile } */


int cond1, cond2;

int foo(int a, int b)
{
  if (cond1)
  {
    if (cond2) return a; return b;  /* { dg-warning "this .if. clause does not guard" } */
  }

  // more code here ...
}

// In that case the fact you have "return return" doesn't stop it being misleading, so maybe the right heuristic here is to continue warning except for one-liners where the entire function body (modulo opening and closing braces) is a single line.


