/* GCC Bug #95145 - internal compiler error: in analyze_functions, at cgraphunit.c:1380 with (invalid) extern nested function
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95145
 */
/* { dg-do compile } */


void foo ( ) 
{ 
 extern __inline int bar ( ) { }

 int baz ( ) { return bar ; } 
}
// Copyright (C) 2020 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//     4 |  extern __inline int bar ( ) { }
// test.c:6:23: warning: returning ‘int (*)()’ from a function with return type ‘int’ makes integer from pointer without a cast []8;;<a href="https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#index-Wint-conversion-Wint-conversion]8">https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#index-Wint-conversion-Wint-conversion]8</a>;;]
//     6 |  int baz ( ) { return bar ; }
// 0x966712 symbol_table::finalize_compilation_unit()
// Copyright (C) 2020 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//     4 |  extern __inline int bar ( ) { }
// test.c:6:23: warning: returning ‘int (*)()’ from a function with return type ‘int’ makes integer from pointer without a cast []8;;<a href="https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#index-Wint-conversion-Wint-conversion]8">https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#index-Wint-conversion-Wint-conversion]8</a>;;]
//     6 |  int baz ( ) { return bar ; }
// 0x963fc2 symbol_table::finalize_compilation_unit()


