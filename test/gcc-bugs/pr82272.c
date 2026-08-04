/* GCC Bug #82272 - RFE: request a warning for (<nonbool> == <bool>) etc.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82272
 */


if (cond == true) {
//         ....
    }
// instead of
    if (cond) {
//         ....
    }
// which has the potential to be disastrous if "cond" is a numeric type which is *not* a bool as these two statements are distinctly different.
// As such, it would be a very good thing if a warning could be emitted in this case.
// C99 and C11 requires that <stdbool.h> define true and false as "the integer constant 1" and "the integer constant 0", respectively, and that those expressions are usable in the preprocessor.  I don't *believe* that that means that they have to expand to the *tokens* 1 and 0, however (and there is always the possibility to do a GNU extension.)

// What I'm suggesting is to do something like _True and _False as cpp-safe equivalent to (_Bool)1 and (_Bool)0, and warn on comparison between a _Bool and a non-_Bool.


