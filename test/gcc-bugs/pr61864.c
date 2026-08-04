/* GCC Bug #61864 - -Wcovered-switch-default to identify "dead" default branch
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61864
 */


#include <stdlib.h> /* for rand() */
int main(void)
{
    int swindex0 = rand();
    enum values {
//         FIRST_ENUMERATOR,
//         SECOND_ENUMERATOR,
//         THIRD_ENUMERATOR
    } swindex1 = SECOND_ENUMERATOR; /* silence unitialized warnings */
//     /* do the non-enumerator switch first: */
    switch (swindex0) {
        case 1:
            swindex1 = FIRST_ENUMERATOR;
            break;
        case 2:
            swindex1 = SECOND_ENUMERATOR;
            break;
        case 3:
            swindex1 = THIRD_ENUMERATOR;
            break;
//         /* -Wswitch-default: usual warning about switch with no default */
    }
    if (swindex0 > 3) {
        swindex1 = THIRD_ENUMERATOR;
    } else if (swindex0 < 1) {
        swindex1 = FIRST_ENUMERATOR;
    }

    switch (swindex1) {
        case FIRST_ENUMERATOR:
            break;
        case SECOND_ENUMERATOR:
            break;
        case THIRD_ENUMERATOR:
            break;
//         /* -Wcovered-switch-default: print a warning here like clang's,
//          * but only if also used with -fstrict-enums: */
        default:
            break;
    }
    switch (swindex1) {
        case FIRST_ENUMERATOR:
            break;
        case SECOND_ENUMERATOR:
            break;
        case THIRD_ENUMERATOR:
            break;
//         /* -Wswitch-default: still warns:
//          * * if by itself, the usual message, as in the first switch.
//          * * if with -Wcovered-switch-default, use a different message,
//          *   like:
//          *   "Warning: switch missing default case which would be covered if it had one"
//          *   "Note: use -fstrict-enums to silence this warning"
//          * * if with -fstrict-enums, do like mentioned in the previous,
//          *   and print no warning. */
    }
    switch (swindex1) {
        case FIRST_ENUMERATOR:
            break;
        case SECOND_ENUMERATOR:
            break;
        case THIRD_ENUMERATOR:
            break;
//         /* Ideally there should be no warning here whatsoever; clang still
//          * warns about it though: */
        default: __builtin_unreachable(); /*NOTREACHED*/
    }
    switch (swindex1) {
        case FIRST_ENUMERATOR:
            break;
        case SECOND_ENUMERATOR:
            break;
        case THIRD_ENUMERATOR: /*FALLTHROUGH*/
//         /* Ideally there should be no warning from -Wcovered-switch-default
//          * here, because even though there are labels for all enum values,
//          * the 3rd case still falls through to the default case: */
        default: /*REACHEDBYFALLTHROUGH*/
            break;
//         /* (clang still warns here though) */
    }
}

// clang version 3.6.0 (tags/RELEASE_360/final)
// Target: x86_64-apple-darwin10.8.0
// Thread model: posix
// switches.c:38:9: warning: default label in switch which covers all enumeration values [-Wcovered-switch-default]
        default:
//         ^
// switches.c:66:9: warning: default label in switch which covers all enumeration values [-Wcovered-switch-default]
        default: __builtin_unreachable(); /*NOTREACHED*/
//         ^
// switches.c:77:9: warning: default label in switch which covers all enumeration values [-Wcovered-switch-default]
        default: /*REACHEDBYFALLTHROUGH*/
//         ^
// 3 warnings generated.
// g++-6_git (GCC) 6.0.0 20150604 (experimental)
// Copyright (C) 2015 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

// switches.c: In function 'int main()':
// switches.c:11:12: warning: switch missing default case [-Wswitch-default]
     switch (swindex0) {
//             ^
// switches.c:41:12: warning: switch missing default case [-Wswitch-default]
     switch (swindex1) {
//             ^
// I'll add the file as an attachment, too.


