/* GCC Bug #71345 - Warn about redundant conditions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71345
 */


{
  if (c != '\0' && c == 'f')
    return 50; 
  return 5;
}
// Here, the first condition seems redundant.


