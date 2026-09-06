/* GCC Bug #93160 - ICE with global register definition after use
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=93160
 */
/* { dg-do compile } */


extern long unsigned int sub_0 ( const char * ) ; 

extern void * sub_1 ( long unsigned int ) ; 

extern int var_0 ; 

void * var_1 = & var_0 ; 

register int var_0 asm ( "%ecx" ) ; 
// Copyright (C) 2019 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    10 | register int var_0 asm ( "%ecx" ) ;
//    10 | register int var_0 asm ( "%ecx" ) ;
// 0xa1b8f9 expand_expr_real_1(tree_node*, rtx_def*, machine_mode, expand_modifier, rtx_def**, bool)
// 0x10636ba assemble_variable(tree_node*, int, int, int)
// 0x1066f29 varpool_node::assemble_decl()
// 0x938c1c symbol_table::compile()
// 0x93ae8c symbol_table::compile()
// 0x93ae8c symbol_table::finalize_compilation_unit()


