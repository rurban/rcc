/* GCC Bug #53182 - GNU C: attributes without underscores should be discouraged / no longer be documented e.g. as examples
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53182
 */


@@ -3233,6 +3233,10 @@ restored before calling the @code{noreturn} function.
//  It does not make sense for a @code{noreturn} function to have a return
 type other than @code{void}.
+The C11 standard defines @code{noreturn} as a macro, in the header
// +@code{<stdnoreturn.h>}, so to avoid conflicting with that macro, the
+reserved name form of the attribute can be used, @code{__noreturn__}.
 @cindex @code{nothrow} function attribute
//  The @code{nothrow} attribute is used to inform the compiler that a


