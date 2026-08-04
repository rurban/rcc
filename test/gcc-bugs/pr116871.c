/* GCC Bug #116871 - -Wincompatible-pointer-types for function pointers could highlight the mismatch better
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116871
 */


typedef void (*__sighandler_t) (int);

    extern __sighandler_t signal (int __sig, __sighandler_t __handler)
         __attribute__ ((__nothrow__ , __leaf__));

    void new_process(void)
    {
      void (*int_stat)();

      int_stat = signal(2,  ((__sighandler_t) 1));

//       signal(2, int_stat);
    }
//     Before this patch, cc1 fails with this message:
//     t.c: In function 'new_process':
//     t.c:18:12: error: assignment to 'void (*)(void)' from incompatible pointer type '__sighandler_t' {aka 'void (*)(int)'} [-Wincompatible-pointer-types]
//        18 |   int_stat = signal(2,  ((__sighandler_t) 1));
//           |            ^
//     t.c:20:13: error: passing argument 2 of 'signal' from incompatible pointer type [-Wincompatible-pointer-types]
//        20 |   signal(2, int_stat);
//           |             ^~~~~~~~
//           |             |
//           |             void (*)(void)
//     t.c:11:57: note: expected '__sighandler_t' {aka 'void (*)(int)'} but argument is of type 'void (*)(void)'
//        11 | extern __sighandler_t signal (int __sig, __sighandler_t __handler)
//           |                                          ~~~~~~~~~~~~~~~^~~~~~~~~
//     With this patch, cc1 emits:
//     t.c: In function 'new_process':
//     t.c:18:12: error: assignment to 'void (*)(void)' from incompatible pointer type '__sighandler_t' {aka 'void (*)(int)'} [-Wincompatible-pointer-types]
//        18 |   int_stat = signal(2,  ((__sighandler_t) 1));
//           |            ^
//     t.c:9:16: note: '__sighandler_t' declared here
//         9 | typedef void (*__sighandler_t) (int);
//           |                ^~~~~~~~~~~~~~
//     t.c:20:13: error: passing argument 2 of 'signal' from incompatible pointer type [-Wincompatible-pointer-types]
//        20 |   signal(2, int_stat);
//           |             ^~~~~~~~
//           |             |
//           |             void (*)(void)
//     t.c:11:57: note: expected '__sighandler_t' {aka 'void (*)(int)'} but argument is of type 'void (*)(void)'
//        11 | extern __sighandler_t signal (int __sig, __sighandler_t __handler)
//           |                                          ~~~~~~~~~~~~~~~^~~~~~~~~
//     t.c:9:16: note: '__sighandler_t' declared here
//         9 | typedef void (*__sighandler_t) (int);
//           |                ^~~~~~~~~~~~~~
//     showing the location of the pertinent typedef ("__sighandler_t")
//     Another example, simplfied from a52dec-0.7.4: src/a52dec.c
//     (rhbz#2336013):

    typedef void (*__sighandler_t) (int);

    extern __sighandler_t signal (int __sig, __sighandler_t __handler)
         __attribute__ ((__nothrow__ , __leaf__));

//     /* Mismatching return type.  */
    static RETSIGTYPE signal_handler (int sig)
    {
    }

    static void print_fps (int final)
    {
      signal (42, signal_handler);
    }
//     Before this patch, cc1 emits:
//     t2.c: In function 'print_fps':
//     t2.c:22:15: error: passing argument 2 of 'signal' from incompatible pointer type [-Wincompatible-pointer-types]
//        22 |   signal (42, signal_handler);
//           |               ^~~~~~~~~~~~~~
//           |               |
//           |               int (*)(int)
//     t2.c:11:57: note: expected '__sighandler_t' {aka 'void (*)(int)'} but argument is of type 'int (*)(int)'
//        11 | extern __sighandler_t signal (int __sig, __sighandler_t __handler)
//           |                                          ~~~~~~~~~~~~~~~^~~~~~~~~
//     With this patch cc1 emits:
//     t2.c: In function 'print_fps':
//     t2.c:22:15: error: passing argument 2 of 'signal' from incompatible pointer type [-Wincompatible-pointer-types]
//        22 |   signal (42, signal_handler);
//           |               ^~~~~~~~~~~~~~
//           |               |
//           |               int (*)(int)
//     t2.c:11:57: note: expected '__sighandler_t' {aka 'void (*)(int)'} but argument is of type 'int (*)(int)'
//        11 | extern __sighandler_t signal (int __sig, __sighandler_t __handler)
//           |                                          ~~~~~~~~~~~~~~~^~~~~~~~~
//     t2.c:16:19: note: 'signal_handler' declared here
//        16 | static RETSIGTYPE signal_handler (int sig)
//           |                   ^~~~~~~~~~~~~~
//     t2.c:9:16: note: '__sighandler_t' declared here
//         9 | typedef void (*__sighandler_t) (int);
//           |                ^~~~~~~~~~~~~~
//     showing the location of the pertinent fndecl ("signal_handler"), and,
//     as before, the pertinent typedef.
//     The patch also updates the colorization in the messages to visually
//     link and contrast the different types and typedefs.
//     My hope is that this make it easier for users to decipher build failures
//     seen with the new C23 default.
//     Further improvements could be made to colorization in
//     convert_for_assignment, and similar improvements to C++, but I'm punting
//     those to GCC 16.
//     gcc/c/ChangeLog:
//             <a class="bz_bug_link 
//           bz_status_ASSIGNED "
//    title="ASSIGNED - -Wincompatible-pointer-types for function pointers could highlight the mismatch better"
//    href="show_bug.cgi?id=116871">PR c/116871</a>
//             * c-typeck.cc (pedwarn_permerror_init): Return bool for whether a
//             warning was emitted.  Only call print_spelling if we warned.
//             (pedwarn_init): Return bool for whether a warning was emitted.
//             (permerror_init): Likewise.
//             (warning_init): Return bool for whether a
//             warning was emitted.  Only call print_spelling if we warned.
//             (class pp_element_quoted_decl): New.
//             (maybe_inform_typedef_location): New.
//             (convert_for_assignment): For OPT_Wincompatible_pointer_types,
//             move auto_diagnostic_group to cover all cases.  Use %e and
//             pp_element rather than %qT and tree to colorize the types.
//             Capture whether a warning was emitted, and, if it was,
//             show various notes: for a pointer to a function, show the
//             function decl, for typedef types, and show the decls.
//     gcc/testsuite/ChangeLog:
//             <a class="bz_bug_link 
//           bz_status_ASSIGNED "
//    title="ASSIGNED - -Wincompatible-pointer-types for function pointers could highlight the mismatch better"
//    href="show_bug.cgi?id=116871">PR c/116871</a>
//             * gcc.dg/c23-mismatching-fn-ptr-a52dec.c: New test.
//             * gcc.dg/c23-mismatching-fn-ptr-alsatools.c: New test.
//             * gcc.dg/c23-mismatching-fn-ptr.c: New test.
//     Signed-off-by: David Malcolm <<a href="mailto:dmalcolm@redhat.com">dmalcolm@redhat.com</a>>


