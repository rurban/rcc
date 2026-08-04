/* GCC Bug #109828 - static compound literal with flexible array in initializer leads to invalid size and ICE
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109828
 */
/* { dg-do compile } */


struct s { int i; char c[]; };

    const struct s s = { .c = "0", };
    const struct s *r = &(constexpr struct s) { .c = "1", };
    const struct s *t = &(static struct s) { .c = "2", };
// Adding a call to __builtin_object_size() and compiling with optimization triggers an ICE

//     size_t ice(void)
    {
        return __builtin_object_size(t, 0);
    }
//        11 |     return __builtin_object_size(t, 0);
//     0xb1023e tree_fits_poly_int64_p(tree_node const*)
//     0xb1023e tree_to_poly_int64(tree_node const*)
//     0x83b5bb component_ref_size(tree_node*, special_array_member*)
//     0x8096ed decl_init_size(tree_node*, bool)
//     0x6d0248 fold_builtin_n(unsigned int, tree_node*, tree_node*, tree_node**, int, bool) [clone .isra.0]
//     0x12084a6 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//     0x1207508 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//     0x1206dfc gimplify_stmt(tree_node**, gimple**)
//     0x1699436 gimplify_and_add(tree_node*, gimple**)
//     0x1208676 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//     0x1206dfc gimplify_stmt(tree_node**, gimple**)


