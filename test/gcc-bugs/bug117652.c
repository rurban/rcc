/* GCC Bug #117652 - [14/15/16/17 regression] ICE: tree check: expected class 'type', have 'exceptional' (error_mark) in tagged_types_tu_compatible_p, at c/c-typeck.cc:1919
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117652
 */
/* { dg-do compile } */

/* Attachment 59626 (reduced testcase): two conflicting definitions of
 * struct foo with different incomplete flexible-array element types used
 * to ICE in tagged_types_tu_compatible_p during error recovery.  Modern
 * gcc errors out cleanly. */
struct foo {
  struct bar array[]; /* { dg-error "array type has incomplete element type" } */
};
struct foo {
  struct bar1 array[]; /* { dg-error "array type has incomplete element type" } */
}; /* { dg-error "redefinition of struct or union .struct foo." } */