/* GCC Bug #118112 - Unhelpful "too many arguments to function" error message (especially w/ C23)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=118112
 */
/* { dg-do compile } */

// Seen attempting to build 389-ds-base with gcc 15 (see linked Red Hat bug
// 2333039).  With -std=gnu23 (the GCC 15 default), a call through a
// zero-argument function-pointer *field* (as opposed to a plain function
// declaration) produced an unhelpful "too many arguments to function"
// diagnostic that didn't say how many arguments were expected/given, nor
// point at the field's declaration.  The reduced testcase from the patch's
// ChangeLog entry:
struct p {
        int (*bar)();
};

void baz() {
    struct p q;
    q.bar(1); /* { dg-error "too many arguments to function .q.bar.; expected 0, have 1" } */
}
//     Before this patch the C frontend emits:
//     t.c: In function 'baz':
//     t.c:7:5: error: too many arguments to function 'q.bar'
//         7 |     q.bar(1);
//           |     ^
//     which doesn't give the user much help in terms of knowing what
//     was expected, and where the relevant declaration is.
//     With this patch the C frontend emits:
//     t.c: In function 'baz':
//     t.c:7:5: error: too many arguments to function 'q.bar'; expected 0, have 1
//         7 |     q.bar(1);
//           |     ^     ~
//     t.c:2:15: note: declared here
//         2 |         int (*bar)();
//           |               ^~~
//     (showing the expected vs actual counts, the pertinent field decl, and
//     underlining the first extraneous argument at the callsite)
//     Similarly, the patch also updates the "too few arguments" case to also
//     show expected vs actual counts.  Doing so requires a tweak to the
//     wording to say "at least" for the case of variadic fns where
//     previously the C FE emitted e.g.:
//     s.c: In function 'test':
//     s.c:5:3: error: too few arguments to function 'callee'
//         5 |   callee ();
//           |   ^~~~~~
//     s.c:1:6: note: declared here
//         1 | void callee (const char *, ...);
//           |      ^~~~~~
//     with this patch it emits:
//     s.c: In function 'test':
//     s.c:5:3: error: too few arguments to function 'callee'; expected at least 1, have 0
//         5 |   callee ();
//           |   ^~~~~~
//     s.c:1:6: note: declared here
//         1 | void callee (const char *, ...);
//           |      ^~~~~~
//     gcc/c/ChangeLog:
//             PR c/118112
//             * c-typeck.cc (inform_declaration): Add "function_expr" param and
//             use it for cases where we couldn't show the function decl to show
//             field decls for callbacks.
//             (build_function_call_vec): Add missing auto_diagnostic_group.
//             Update for new param of inform_declaration.
//             (convert_arguments): Likewise.  For the "too many arguments" case
//             add the expected vs actual counts to the message, and if we have
//             it, add the location_t of the first surplus param as a secondary
//             location within the diagnostic.  For the "too few arguments" case,
//             determine the minimum number of arguments required and add the
//             expected vs actual counts to the message, tweaking it to "at least"
//             for variadic functions.
//     gcc/testsuite/ChangeLog:
//             PR c/118112
//             * gcc.dg/too-few-arguments.c: New test.
//             * gcc.dg/too-many-arguments.c: New test.
//     Signed-off-by: David Malcolm <dmalcolm@redhat.com>
