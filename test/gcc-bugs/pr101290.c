/* GCC Bug #101290 - ICE with -O1 on valid code: in maybe_canonicalize_mem_ref_addr, at gimple-fold.c:5976
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101290
 */
/* { dg-do compile } */


case COMPONENT_REF:
          {
            tree field = TREE_OPERAND (exp, 1);
            tree this_offset = component_ref_field_offset (exp);
            poly_int64 hthis_offset;

            if (!this_offset
//                 || !poly_int_tree_p (this_offset, &hthis_offset)
//                 || (TREE_INT_CST_LOW (DECL_FIELD_BIT_OFFSET (field))
//                     % BITS_PER_UNIT))
              return NULL_TREE;
// because this_offset doesn't fit the signed poly_int64.  IIRC we do have to
// support negative field offsets.
// Eventually this testcase is invalid since sizeof (*g) is bigger than half
// of the address space.  Joseph?  We seem to happily wrap TYPE_SIZE[_UNIT]
// even over the sizetype bounds without diagnosing anything - we do emit
// some diagnostics from layout_type so that might be the place to complain
// (we could then set TYPE_SIZE[_UNIT] to error_mark_node).
//  <record_type 0x7ffff6677dc8 type_0 BLK
//     size <integer_cst 0x7ffff66862a0 type <integer_type 0x7ffff655c0a8 bitsizetype> constant public overflow 0x704ef12cb04cf1580>
//     unit-size <integer_cst 0x7ffff6686260 type <integer_type 0x7ffff655c000 sizetype> constant public overflow 16185341509340095152>


