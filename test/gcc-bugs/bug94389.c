/* GCC Bug #94389 - __attribute__((warn_unused_result)) will warn if the result is discarded as an optimisation
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94389
 */
/* { dg-do compile } */


#define O_TEXT 0  /* defined on Windows and DOS, but not needed on Unix */

    __attribute__((warn_unused_result))
    extern int text_mode(void);

    int get_flags(void) {
        return text_mode() ? O_TEXT : 0;
    }

// Similarly, there is a warning if the result of a function marked with __attribute__((warn_unused_result)) is multiplied by 0, and probably with other expressions that are easily constant-folded. If the function also has __attribute__((const)), there is no warning.

//    title="UNCONFIRMED - (void) cast doesn't suppress __attribute__((warn_unused_result))"


