# TCC Test Suite Report for RCC

Generated on: 05/26/2026 10:05:00

## Summary

- **Total Tests**: 115
- **Passed**: 87
- **Failed**: 28
- **Pass Rate**: 75.65%

## Detailed Results

| Test                        | Status       | Message                       |
| --------------------------- | ------------ | ----------------------------- |
| 00_assignment               | PASS         | Output matches                |
| 01_comment                  | PASS         | Output matches                |
| 02_printf                   | PASS         | Output matches                |
| 03_struct                   | PASS         | Output matches                |
| 04_for                      | PASS         | Output matches                |
| 05_array                    | PASS         | Output matches                |
| 06_case                     | PASS         | Output matches                |
| 07_function                 | PASS         | Output matches                |
| 08_while                    | PASS         | Output matches                |
| 09_do_while                 | PASS         | Output matches                |
| 10_pointer                  | PASS         | Output matches                |
| 11_precedence               | PASS         | Output matches                |
| 12_hashdefine               | PASS         | Output matches                |
| 13_integer_literals         | PASS         | Output matches                |
| 14_if                       | PASS         | Output matches                |
| 15_recursion                | PASS         | Output matches                |
| 16_nesting                  | PASS         | Output matches                |
| 17_enum                     | MISMATCH     | Output does not match .expect |
| 18_include                  | PASS         | Output matches                |
| 19_pointer_arithmetic       | PASS         | Output matches                |
| 20_pointer_comparison       | PASS         | Output matches                |
| 21_char_array               | PASS         | Output matches                |
| 22_floating_point           | EXEC_FAIL    | exited with code -1073741819  |
| 23_type_coercion            | PASS         | Output matches                |
| 24_math_library             | MISMATCH     | Output does not match .expect |
| 25_quicksort                | PASS         | Output matches                |
| 26_character_constants      | PASS         | Output matches                |
| 27_sizeof                   | PASS         | Output matches                |
| 28_strings                  | PASS         | Output matches                |
| 29_array_address            | PASS         | Output matches                |
| 30_hanoi                    | PASS         | Output matches                |
| 31_args                     | PASS         | Output matches                |
| 32_led                      | PASS         | Output matches                |
| 33_ternary_op               | EXEC_FAIL    | exited with code -1073741819  |
| 34_array_assignment         | MISMATCH     | Output does not match .expect |
| 35_sizeof                   | PASS         | Output matches                |
| 36_array_initialisers       | PASS         | Output matches                |
| 37_sprintf                  | PASS         | Output matches                |
| 38_multiple_array_index     | PASS         | Output matches                |
| 39_typedef                  | PASS         | Output matches                |
| 40_stdio                    | PASS         | Output matches                |
| 41_hashif                   | PASS         | Output matches                |
| 42_function_pointer         | PASS         | Output matches                |
| 43_void_param               | PASS         | Output matches                |
| 44_scoped_declarations      | PASS         | Output matches                |
| 45_empty_for                | PASS         | Output matches                |
| 46_grep                     | EXEC_FAIL    | exited with code -1073741819  |
| 47_switch_return            | PASS         | Output matches                |
| 48_nested_break             | PASS         | Output matches                |
| 49_bracket_evaluation       | COMPILE_FAIL | rcc exited with 1             |
| 50_logical_second_arg       | PASS         | Output matches                |
| 51_static                   | EXEC_FAIL    | exited with code -1073741819  |
| 52_unnamed_enum             | PASS         | Output matches                |
| 54_goto                     | PASS         | Output matches                |
| 55_lshift_type              | EXEC_FAIL    | exited with code -1073741819  |
| 60_errors_and_warnings      | SKIP         | Skipped                       |
| 61_integers                 | PASS         | Output matches                |
| 64_macro_nesting            | PASS         | Output matches                |
| 67_macro_concat             | PASS         | Output matches                |
| 70_floating_point_literals  | MISMATCH     | Output does not match .expect |
| 71_macro_empty_arg          | PASS         | Output matches                |
| 72_long_long_constant       | PASS         | Output matches                |
| 73_arm64                    | SKIP         | Skipped                       |
| 75_array_in_struct_init     | PASS         | Output matches                |
| 76_dollars_in_identifiers   | PASS         | Output matches                |
| 77_push_pop_macro           | PASS         | Output matches                |
| 78_vla_label                | PASS         | Output matches                |
| 79_vla_continue             | PASS         | Output matches                |
| 80_flexarray                | PASS         | Output matches                |
| 81_types                    | PASS         | Output matches                |
| 82_attribs_position         | PASS         | Output matches                |
| 83_utf8_in_identifiers      | PASS         | Output matches                |
| 84_hex-float                | PASS         | Output matches                |
| 85_asm-outside-function     | PASS         | Output matches                |
| 86_memory-model             | PASS         | Output matches                |
| 87_dead_code                | PASS         | Output matches                |
| 88_codeopt                  | PASS         | Output matches                |
| 89_nocode_wanted            | PASS         | Output matches                |
| 90_struct-init              | EXEC_FAIL    | exited with code -1073740791  |
| 91_ptr_longlong_arith32     | PASS         | Output matches                |
| 92_enum_bitfield            | PASS         | Output matches                |
| 93_integer_promotion        | PASS         | Output matches                |
| 94_generic                  | PASS         | Output matches                |
| 95_bitfields                | MISMATCH     | Output does not match .expect |
| 95_bitfields_ms             | MISMATCH     | Output does not match .expect |
| 96_nodata_wanted            | SKIP         | Skipped                       |
| 97_utf8_string_literal      | PASS         | Output matches                |
| 98_al_ax_extend             | SKIP         | Skipped                       |
| 99_fastcall                 | SKIP         | Skipped                       |
| 100_c99array-decls          | PASS         | Output matches                |
| 101_cleanup                 | EXEC_FAIL    | exited with code -1073741819  |
| 102_alignas                 | MISMATCH     | Output does not match .expect |
| 103_implicit_memmove        | PASS         | Output matches                |
| 104_inline                  | SKIP         | Skipped                       |
| 105_local_extern            | PASS         | Output matches                |
| 106_versym                  | MISMATCH     | Output does not match .expect |
| 107_stack_safe              | MISMATCH     | Output does not match .expect |
| 108_constructor             | MISMATCH     | Output does not match .expect |
| 109_float_struct_calling    | PASS         | Output matches                |
| 110_average                 | PASS         | Output matches                |
| 111_conversion              | PASS         | Output matches                |
| 112_backtrace               | SKIP         | Skipped                       |
| 113_btdll                   | SKIP         | Skipped                       |
| 114_bound_signal            | SKIP         | Skipped                       |
| 115_bound_setjmp            | SKIP         | Skipped                       |
| 116_bound_setjmp2           | SKIP         | Skipped                       |
| 117_builtins                | PASS         | Output matches                |
| 118_switch                  | PASS         | Output matches                |
| 119_random_stuff            | EXEC_FAIL    | exited with code -1073741819  |
| 120_alias                   | SKIP         | Skipped                       |
| 121_struct_return           | MISMATCH     | Output does not match .expect |
| 122_vla_reuse               | PASS         | Output matches                |
| 123_vla_bug                 | EXEC_FAIL    | exited with code -1073741819  |
| 124_atomic_counter          | MISMATCH     | Output does not match .expect |
| 125_atomic_misc             | SKIP         | Skipped                       |
| 126_bound_global            | SKIP         | Skipped                       |
| 127_asm_goto                | PASS         | Output matches                |
| 128_run_atexit              | SKIP         | Skipped                       |
| 129_scopes                  | EXEC_FAIL    | exited with code -1073741819  |
| 130_large_argument          | MISMATCH     | Output does not match .expect |
| 131_return_struct_in_reg    | MISMATCH     | Output does not match .expect |
| 132_bound_test              | PASS         | Output matches                |
| 133_old_func                | PASS         | Output matches                |
| 134_double_to_signed        | PASS         | Output matches                |
| 135_func_arg_struct_compare | PASS         | Output matches                |
| 136_atomic_gcc_style        | PASS         | Output matches                |
| 137_funcall_struct_args     | MISMATCH     | Output does not match .expect |
| 138_arm64_encoding          | SKIP         | Skipped                       |
| 138_narrow_return_promotion | MISMATCH     | Output does not match .expect |
| 139_arm64_errors            | SKIP         | Skipped                       |
| 139_narrow_type_conversion  | MISMATCH     | Output does not match .expect |
| 140_arm64_extasm            | SKIP         | Skipped                       |
| 140_int_sign_extension      | PASS         | Output matches                |
| 141_riscv_asm_pseudo        | SKIP         | Skipped                       |
| 142_riscv_asm_longlong      | SKIP         | Skipped                       |
| 143_riscv_asm_farith        | SKIP         | Skipped                       |
