/* GCC Bug #99198 - when combinating nested function and __builtin_call_with_static_chain, optimization triggers an internal compiler error (verify_gimple)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99198
 */
/* { dg-do compile } */
/* { dg-options "-O1" } */


int main() {
    void f() {}
    __builtin_call_with_static_chain(f(), &f);

    return 0;
}

// Copyright (C) 2021 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//     1 | int main() {
// f (); [static-chain: f]
// 0x1a31d99 internal_error(char const*, ...)
// 0xdc218d verify_gimple_in_seq(gimple*)


