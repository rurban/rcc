/* GCC Bug #124305 - btf_decl_tag and btf_type_tag attributes give compiler errors in C++  and with -Wwrite-strings in C
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124305
 */


// /* Only narrow character strings are accepted.  */
  tree argtype = TREE_TYPE (TREE_TYPE (TREE_VALUE (args)));
  if (!(argtype == char_type_node
//         || argtype == char8_type_node
//         || argtype == signed_char_type_node
//         || argtype == unsigned_char_type_node))
    {
//       error ("unsupported wide string type argument in %qE attribute", name);
      return false;
    }
// ```
// That is argtype should be `argtype = TYPE_MAIN_VARIANT (argtype);` before the check.


