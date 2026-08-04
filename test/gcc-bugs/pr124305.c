/* GCC Bug #124305 - btf_decl_tag and btf_type_tag attributes give compiler errors in C++  and with -Wwrite-strings in C
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124305
 */
/* { dg-do compile } */
/* { dg-options "-Wwrite-strings" } */

// Testcase from comment 0 (the btf_decl_tag/btf_type_tag examples from the
// GCC manual).  In C++ these are rejected outright; in C they only fail
// when -Wwrite-strings is used (which makes string literals "const char *",
// tripping the "narrow character strings only" check in
// handle_btf_decl_tag_attribute/handle_btf_type_tag_attribute since it
// compares against the non-const char_type_node instead of using
// TYPE_MAIN_VARIANT (see comment 3).
int *foo1 __attribute__ ((btf_decl_tag ("__percpu"))); /* { dg-error "unsupported wide string type argument in .btf_decl_tag. attribute" } */
int * __attribute__ ((btf_type_tag ("__user"))) bar1; /* { dg-error "unsupported wide string type argument in .btf_type_tag. attribute" } */

int *foo2 [[gnu::btf_decl_tag ("__percpu")]]; /* { dg-error "unsupported wide string type argument in .btf_decl_tag. attribute" } */
int * [[gnu::btf_type_tag ("__user")]] bar2; /* { dg-error "unsupported wide string type argument in .btf_type_tag. attribute" } */

// Root cause (comment 3): handle_btf_*_attribute() computes
//   tree argtype = TREE_TYPE (TREE_TYPE (TREE_VALUE (args)));
// and compares argtype directly against char_type_node/char8_type_node/
// signed_char_type_node/unsigned_char_type_node, rejecting it as "wide"
// whenever it is actually a qualified (const) variant of one of those,
// e.g. under -Wwrite-strings or in C++.  Fix: argtype should be run through
// TYPE_MAIN_VARIANT (argtype) before the comparison.


