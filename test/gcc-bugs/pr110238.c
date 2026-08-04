/* GCC Bug #110238 - Incorrect "comparison between pointer and zero character constant" warning when comparing pointer to unsigned null pointer constant since r7-5677-ga9342885b149
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110238
 */


static bool
// char_type_p (tree type)
{
  return (type == char_type_node
//          || type == unsigned_char_type_node
//          || type == signed_char_type_node
//          || type == char16_type_node
//          || type == char32_type_node);
}
// ```
// In C, there is no distinct type for char32_type_node so it is the same as unsigned here and it returns true for that case .


