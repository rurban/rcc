/* GCC Bug #53182 - GNU C: attributes without underscores should be discouraged / no longer be documented e.g. as examples
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53182
 */
/* { dg-do compile } */

/* Attribute names without a leading/trailing double underscore are
   ordinary identifiers and can clash with macros defined by system
   headers.  The C11 header <stdnoreturn.h> defines "noreturn" as a
   macro (expanding to "_Noreturn"), so using the unprefixed attribute
   name after including it silently expands to something that is no
   longer a recognized attribute name: */
#include <stdnoreturn.h>

void fatal (void) __attribute__ ((noreturn));

/* The reserved name form is not affected by the macro and should
   always be preferred in code that may be combined with
   <stdnoreturn.h>: */
void fatal2 (void) __attribute__ ((__noreturn__));

// Requested documentation fix (comment 7), for gcc/doc/extend.texi:
//
// --- a/gcc/doc/extend.texi
// +++ b/gcc/doc/extend.texi
// @@ -3233,6 +3233,10 @@ restored before calling the @code{noreturn} function.
//  It does not make sense for a @code{noreturn} function to have a return
//  type other than @code{void}.
//
// +The C11 standard defines @code{noreturn} as a macro, in the header
// +@code{<stdnoreturn.h>}, so to avoid conflicting with that macro, the
// +reserved name form of the attribute can be used, @code{__noreturn__}.
// +
//  @item nothrow
//  @cindex @code{nothrow} function attribute
//  The @code{nothrow} attribute is used to inform the compiler that a
//
// Eric Blake's suggestion (comment 2) was that GCC could also recognize
// __attribute__((_Noreturn)) so that __attribute__((noreturn)) still
// works even after <stdnoreturn.h> replaces it with the macro.
