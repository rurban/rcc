/* GCC Bug #103104 - missing warning about superfluous forward declaration -Wsuperfluous-forward-declaration
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103104
 */
/* { dg-do compile } */
/* { dg-options "-Wall -Wextra -Wredundant-decls" } */

/* -Wredundant-decls warns on a repeated non-defining declaration but
 * misses the first superfluous forward declaration of a function that
 * is then defined, as documented (comment 2 of the bug). */
static void f (void);
static void f (void);      /* { dg-warning "redundant redeclaration of .f." } */
static void f (void) { }

static void g (void);      /* no warning - the missing case */
static void g (void) { }

void h (void)
{
  f ();
  g ();
}


