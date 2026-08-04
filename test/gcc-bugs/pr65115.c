/* GCC Bug #65115 - default init_priority attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65115
 */
/* { dg-do compile } */


@item init_priority (@var{priority})
@cindex @code{init_priority} variable attribute
@emph{in a given translation unit}.  No guarantee is made for initializations
@code{init_priority} attribute by specifying a relative @var{priority},
// In the following example, @code{A} would normally be created before
// @code{B}, but the @code{init_priority} attribute reverses that order:


