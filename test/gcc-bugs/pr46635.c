/* GCC Bug #46635 - c-family/c-common.c uses BITS_PER_UNIT in lieu of TYPE_PRECISION (char_type_node)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=46635
 */


{
// +  tree size = fold_offsetof_1 (expr, stop_ref);
// +
// +  /* Convert in case a char is more than one unit.  */
// +  size
// +    = size_binop (CEIL_DIV_EXPR, size,
// +                 size_int (TYPE_PRECISION (char_type_node) / BITS_PER_UNIT));
//    /* Convert back from the internal sizetype to size_t.  */
// -  return convert (size_type_node, fold_offsetof_1 (expr, stop_ref));
// +  return convert (size_type_node, size);
 }

//  /* Warn for A ?: C expressions (with B omitted) where A is a boolean


