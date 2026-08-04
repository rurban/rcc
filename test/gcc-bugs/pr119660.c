/* GCC Bug #119660 - builtin functions erroneously suggested as originating in system headers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119660
 */


void __builtin_abort (int[foo]);

// gcc.dg/Wbuiltin-declaration-mismatch-16.c:5:27: error: 'foo' undeclared here (not in a function)
// gcc.dg/Wbuiltin-declaration-mismatch-16.c:5:6: warning: conflicting types for built-in function '__builtin_abort'; expected 'void(void)' [-Wbuiltin-declaration-mismatch]


