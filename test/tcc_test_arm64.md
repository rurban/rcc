# TCC Test Suite Report for RCC

Generated: June 2026

## Summary

- **Total**: 120
- **Passed**: 1
- **Failed**: 119
- **Pass Rate**: 0%

## Detailed Results

| Test                        | Status       | Message               |
| --------------------------- | ------------ | --------------------- |
| 60_errors_and_warnings      | SKIP         | Skipped               |
| 95_bitfields_ms             | SKIP         | Skipped               |
| 96_nodata_wanted            | SKIP         | Skipped               |
| 98_al_ax_extend             | SKIP         | Skipped               |
| 99_fastcall                 | SKIP         | Skipped               |
| 112_backtrace               | SKIP         | Skipped               |
| 113_btdll                   | SKIP         | Skipped               |
| 114_bound_signal            | SKIP         | Skipped               |
| 115_bound_setjmp            | SKIP         | Skipped               |
| 116_bound_setjmp2           | SKIP         | Skipped               |
| 120_alias                   | SKIP         | Skipped               |
| 126_bound_global            | SKIP         | Skipped               |
| 127_asm_goto                | SKIP         | Skipped               |
| 141_riscv_asm               | SKIP         | Skipped               |
| 145_winarm64_interlocked    | SKIP         | Skipped               |
| 00_assignment               | EXEC_FAIL    | non-zero exit         |
| 01_comment                  | EXEC_FAIL    | non-zero exit         |
| 02_printf                   | EXEC_FAIL    | non-zero exit         |
| 03_struct                   | EXEC_FAIL    | non-zero exit         |
| 04_for                      | EXEC_FAIL    | non-zero exit         |
| 05_array                    | EXEC_FAIL    | non-zero exit         |
| 06_case                     | EXEC_FAIL    | non-zero exit         |
| 07_function                 | EXEC_FAIL    | non-zero exit         |
| 08_while                    | EXEC_FAIL    | non-zero exit         |
| 09_do_while                 | EXEC_FAIL    | non-zero exit         |
| 10_pointer                  | EXEC_FAIL    | non-zero exit         |
| 11_precedence               | EXEC_FAIL    | non-zero exit         |
| 12_hashdefine               | EXEC_FAIL    | non-zero exit         |
| 13_integer_literals         | EXEC_FAIL    | non-zero exit         |
| 14_if                       | EXEC_FAIL    | non-zero exit         |
| 15_recursion                | EXEC_FAIL    | non-zero exit         |
| 16_nesting                  | EXEC_FAIL    | non-zero exit         |
| 17_enum                     | EXEC_FAIL    | non-zero exit         |
| 18_include                  | EXEC_FAIL    | non-zero exit         |
| 19_pointer_arithmetic       | EXEC_FAIL    | non-zero exit         |
| 20_pointer_comparison       | EXEC_FAIL    | non-zero exit         |
| 21_char_array               | EXEC_FAIL    | non-zero exit         |
| 22_floating_point           | EXEC_FAIL    | non-zero exit         |
| 23_type_coercion            | EXEC_FAIL    | non-zero exit         |
| 24_math_library             | EXEC_FAIL    | non-zero exit         |
| 25_quicksort                | EXEC_FAIL    | non-zero exit         |
| 26_character_constants      | EXEC_FAIL    | non-zero exit         |
| 27_sizeof                   | EXEC_FAIL    | non-zero exit         |
| 28_strings                  | EXEC_FAIL    | non-zero exit         |
| 29_array_address            | EXEC_FAIL    | non-zero exit         |
| 30_hanoi                    | EXEC_FAIL    | non-zero exit         |
| 31_args                     | EXEC_FAIL    | non-zero exit         |
| 32_led                      | EXEC_FAIL    | non-zero exit         |
| 33_ternary_op               | EXEC_FAIL    | non-zero exit         |
| 34_array_assignment         | EXEC_FAIL    | non-zero exit         |
| 35_sizeof                   | EXEC_FAIL    | non-zero exit         |
| 36_array_initialisers       | EXEC_FAIL    | non-zero exit         |
| 37_sprintf                  | EXEC_FAIL    | non-zero exit         |
| 38_multiple_array_index     | EXEC_FAIL    | non-zero exit         |
| 39_typedef                  | EXEC_FAIL    | non-zero exit         |
| 40_stdio                    | EXEC_FAIL    | non-zero exit         |
| 41_hashif                   | EXEC_FAIL    | non-zero exit         |
| 42_function_pointer         | EXEC_FAIL    | non-zero exit         |
| 43_void_param               | EXEC_FAIL    | non-zero exit         |
| 44_scoped_declarations      | EXEC_FAIL    | non-zero exit         |
| 45_empty_for                | EXEC_FAIL    | non-zero exit         |
| 47_switch_return            | EXEC_FAIL    | non-zero exit         |
| 48_nested_break             | EXEC_FAIL    | non-zero exit         |
| 49_bracket_evaluation       | EXEC_FAIL    | non-zero exit         |
| 50_logical_second_arg       | EXEC_FAIL    | non-zero exit         |
| 51_static                   | EXEC_FAIL    | non-zero exit         |
| 52_unnamed_enum             | EXEC_FAIL    | non-zero exit         |
| 54_goto                     | EXEC_FAIL    | non-zero exit         |
| 55_lshift_type              | EXEC_FAIL    | non-zero exit         |
| 61_integers                 | EXEC_FAIL    | non-zero exit         |
| 64_macro_nesting            | EXEC_FAIL    | non-zero exit         |
| 67_macro_concat             | EXEC_FAIL    | non-zero exit         |
| 70_floating_point_literals  | EXEC_FAIL    | non-zero exit         |
| 71_macro_empty_arg          | EXEC_FAIL    | non-zero exit         |
| 72_long_long_constant       | EXEC_FAIL    | non-zero exit         |
| 73_arm64                    | EXEC_FAIL    | non-zero exit         |
| 75_array_in_struct_init     | EXEC_FAIL    | non-zero exit         |
| 76_dollars_in_identifiers   | EXEC_FAIL    | non-zero exit         |
| 77_push_pop_macro           | EXEC_FAIL    | non-zero exit         |
| 78_vla_label                | EXEC_FAIL    | non-zero exit         |
| 79_vla_continue             | EXEC_FAIL    | non-zero exit         |
| 80_flexarray                | EXEC_FAIL    | non-zero exit         |
| 81_types                    | EXEC_FAIL    | non-zero exit         |
| 82_attribs_position         | EXEC_FAIL    | non-zero exit         |
| 83_utf8_in_identifiers      | EXEC_FAIL    | non-zero exit         |
| 84_hex-float                | EXEC_FAIL    | non-zero exit         |
| 85_asm-outside-function     | EXEC_FAIL    | non-zero exit         |
| 86_memory-model             | EXEC_FAIL    | non-zero exit         |
| 87_dead_code                | EXEC_FAIL    | non-zero exit         |
| 88_codeopt                  | EXEC_FAIL    | non-zero exit         |
| 89_nocode_wanted            | EXEC_FAIL    | non-zero exit         |
| 90_struct-init              | EXEC_FAIL    | non-zero exit         |
| 91_ptr_longlong_arith32     | EXEC_FAIL    | non-zero exit         |
| 92_enum_bitfield            | EXEC_FAIL    | non-zero exit         |
| 93_integer_promotion        | EXEC_FAIL    | non-zero exit         |
| 94_generic                  | EXEC_FAIL    | non-zero exit         |
| 95_bitfields                | EXEC_FAIL    | non-zero exit         |
| 97_utf8_string_literal      | EXEC_FAIL    | non-zero exit         |
| 100_c99array-decls          | EXEC_FAIL    | non-zero exit         |
| 101_cleanup                 | EXEC_FAIL    | non-zero exit         |
| 102_alignas                 | EXEC_FAIL    | non-zero exit         |
| 103_implicit_memmove        | EXEC_FAIL    | non-zero exit         |
| 104_inline                  | EXEC_FAIL    | non-zero exit         |
| 105_local_extern            | EXEC_FAIL    | non-zero exit         |
| 106_versym                  | EXEC_FAIL    | non-zero exit         |
| 107_stack_safe              | EXEC_FAIL    | non-zero exit         |
| 108_constructor             | EXEC_FAIL    | non-zero exit         |
| 109_float_struct_calling    | EXEC_FAIL    | non-zero exit         |
| 110_average                 | EXEC_FAIL    | non-zero exit         |
| 111_conversion              | EXEC_FAIL    | non-zero exit         |
| 117_builtins                | EXEC_FAIL    | non-zero exit         |
| 118_switch                  | EXEC_FAIL    | non-zero exit         |
| 119_random_stuff            | EXEC_FAIL    | non-zero exit         |
| 121_struct_return           | EXEC_FAIL    | non-zero exit         |
| 122_vla_reuse               | EXEC_FAIL    | non-zero exit         |
| 123_vla_bug                 | EXEC_FAIL    | non-zero exit         |
| 124_atomic_counter          | EXEC_FAIL    | non-zero exit         |
| 128_run_atexit              | MISMATCH     | Output differs        |
| 130_large_argument          | EXEC_FAIL    | non-zero exit         |
| 131_return_struct_in_reg    | EXEC_FAIL    | non-zero exit         |
| 132_bound_test              | EXEC_FAIL    | non-zero exit         |
| 133_old_func                | EXEC_FAIL    | non-zero exit         |
| 134_double_to_signed        | EXEC_FAIL    | non-zero exit         |
| 135_func_arg_struct_compare | EXEC_FAIL    | non-zero exit         |
| 136_atomic_gcc_style        | EXEC_FAIL    | non-zero exit         |
| 137_funcall_struct_args     | EXEC_FAIL    | non-zero exit         |
| 138_arm64_encoding          | EXEC_FAIL    | non-zero exit         |
| 140_arm64_extasm            | EXEC_FAIL    | non-zero exit         |
| 142_int_conversion          | EXEC_FAIL    | non-zero exit         |
| 143_void_expr               | EXEC_FAIL    | non-zero exit         |
| 144_tls                     | COMPILE_FAIL | rcc returned non-zero |
| 46_grep                     | EXEC_FAIL    | non-zero exit         |
| 125_atomic_misc             | MISMATCH     | Output differs        |
| 129_scopes                  | EXEC_FAIL    | non-zero exit         |
| 139_arm64_errors            | PASS         | Output matches        |
