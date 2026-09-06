/* GCC Bug #117289 - gcc.dg/debug/ctf/ctf-function-pointers-2.c failure with -std=gnu23
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117289
 */
/* { dg-do compile } */


// I'm not sure how exactly CTF is meant to handle deduplication where there are multiple equivalent versions of the same type (POINTER_TYPE, FUNCTION_TYPE etc.) that are supposed to end up the same in output. Here, we have two versions of the function pointer type void (*) (struct callback_head *head): one for the member of the structure, and one in the subsequently defined typedef.
// DWARF debug info seems to get things right even with -std=gnu23: the debug info for the typedef points to the same type as used for the structure member. However, CTF outputs the pointer and function types twice, so resulting in the failure of this test with -std=gnu23.
// An actual difference between the two function type definitions is that the one for the member has TYPE_STRUCTURAL_EQUALITY_P set, whereas that for the typedef has TYPE_CANONICAL set. It appears, judging by the comment on c_update_type_canonical, that it may be deliberate for a type created while the structure was incomplete to retain TYPE_STRUCTURAL_EQUALITY_P. I don't know if this difference is the actual cause of the problem, or if something else is meant to ensure that types with no semantic difference as far as CTF is concerned don't end up duplicated in the CTF output, even if there are multiple type nodes at the tree level (as noted above, it seems that DWARF ends up getting the output right, without duplication).


