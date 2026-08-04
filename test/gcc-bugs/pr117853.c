/* GCC Bug #117853 - "warning: '?:' using integer constants in boolean context, the expression will always evaluate to 'true' [-Wint-in-bool-context]" but no ?: in sight
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117853
 */


_Bool f(int a) {
  return 1 + !!a;
}
// ```


