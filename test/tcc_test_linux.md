# TCC Test Suite Report for RCC

Generated: May 2026

## Summary

- **Total**: 152
- **Passed**: 43
- **Failed**: 109
- **Pass Rate**: 28%

## Detailed Results

| Test                        | Status       | Message                       |
| :-------------------------- | :----------- | :---------------------------- |
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
| 11_precedence               | MISMATCH     | Output does not match .expect |
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
| 22_floating_point           | EXEC_FAIL    | non-zero exit                 |
| 23_type_coercion            | EXEC_FAIL    | non-zero exit                 |
| 24_math_library             | EXEC_FAIL    | non-zero exit                 |
| 25_quicksort                | PASS         | Output matches                |
| 26_character_constants      | PASS         | Output matches                |
| 27_sizeof                   | PASS         | Output matches                |
| 28_strings                  | EXEC_FAIL    | non-zero exit                 |
| 29_array_address            | PASS         | Output matches                |
| 30_hanoi                    | PASS         | Output matches                |
| 31_args                     | PASS         | Output matches                |
| 32_led                      | EXEC_FAIL    | non-zero exit                 |
| 33_ternary_op               | EXEC_FAIL    | non-zero exit                 |
| 34_array_assignment         | EXEC_FAIL    | non-zero exit                 |
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
| 46_grep                     | MISMATCH     | Output does not match .expect |
| 47_switch_return            | PASS         | Output matches                |
| 48_nested_break             | PASS         | Output matches                |
| 49_bracket_evaluation       | MISMATCH     | Output does not match .expect |
| 50_logical_second_arg       | MISMATCH     | Output does not match .expect |
| 51_static                   | PASS         | Output matches                |
| 52_unnamed_enum             | PASS         | Output matches                |
| 54_goto                     | PASS         | Output matches                |
| 55_lshift_type              | EXEC_FAIL    | non-zero exit                 |
| 60_errors_and_warnings      | SKIP         | Skipped                       |
| 61_integers                 | COMPILE_FAIL | rcc returned non-zero         |
| 64_macro_nesting            | COMPILE_FAIL | rcc returned non-zero         |
| 67_macro_concat             | COMPILE_FAIL | rcc returned non-zero         |
| 70_floating_point_literals  | COMPILE_FAIL | rcc returned non-zero         |
| 71_macro_empty_arg          | COMPILE_FAIL | rcc returned non-zero         |
| 72_long_long_constant       | COMPILE_FAIL | rcc returned non-zero         |
| 73_arm64                    | SKIP         | Skipped                       |
| 75_array_in_struct_init     | COMPILE_FAIL | rcc returned non-zero         |
| 76_dollars_in_identifiers   | COMPILE_FAIL | rcc returned non-zero         |
| 77_push_pop_macro           | COMPILE_FAIL | rcc returned non-zero         |
| 78_vla_label                | COMPILE_FAIL | rcc returned non-zero         |
| 79_vla_continue             | COMPILE_FAIL | rcc returned non-zero         |
| 80_flexarray                | COMPILE_FAIL | rcc returned non-zero         |
| 81_types                    | COMPILE_FAIL | rcc returned non-zero         |
| 82_attribs_position         | COMPILE_FAIL | rcc returned non-zero         |
| 83_utf8_in_identifiers      | COMPILE_FAIL | rcc returned non-zero         |
| 84_hex-float                | COMPILE_FAIL | rcc returned non-zero         |
| 85_asm-outside-function     | COMPILE_FAIL | rcc returned non-zero         |
| 86_memory-model             | COMPILE_FAIL | rcc returned non-zero         |
| 87_dead_code                | COMPILE_FAIL | rcc returned non-zero         |
| 88_codeopt                  | COMPILE_FAIL | rcc returned non-zero         |
| 89_nocode_wanted            | COMPILE_FAIL | rcc returned non-zero         |
| 90_struct-init              | COMPILE_FAIL | rcc returned non-zero         |
| 91_ptr_longlong_arith32     | COMPILE_FAIL | rcc returned non-zero         |
| 92_enum_bitfield            | COMPILE_FAIL | rcc returned non-zero         |
| 93_integer_promotion        | COMPILE_FAIL | rcc returned non-zero         |
| 94_generic                  | COMPILE_FAIL | rcc returned non-zero         |
| 95_bitfields                | COMPILE_FAIL | rcc returned non-zero         |
| 95_bitfields_ms             | COMPILE_FAIL | rcc returned non-zero         |
| 96_nodata_wanted            | SKIP         | Skipped                       |
| 97_utf8_string_literal      | COMPILE_FAIL | rcc returned non-zero         |
| 98_al_ax_extend             | SKIP         | Skipped                       |
| 99_fastcall                 | SKIP         | Skipped                       |
| 100_c99array-decls          | COMPILE_FAIL | rcc returned non-zero         |
| 101_cleanup                 | COMPILE_FAIL | rcc returned non-zero         |
| 102_alignas                 | COMPILE_FAIL | rcc returned non-zero         |
| 103_implicit_memmove        | COMPILE_FAIL | rcc returned non-zero         |
| 104_inline                  | COMPILE_FAIL | rcc returned non-zero         |
| 105_local_extern            | COMPILE_FAIL | rcc returned non-zero         |
| 106_versym                  | COMPILE_FAIL | rcc returned non-zero         |
| 107_stack_safe              | COMPILE_FAIL | rcc returned non-zero         |
| 108_constructor             | COMPILE_FAIL | rcc returned non-zero         |
| 109_float_struct_calling    | COMPILE_FAIL | rcc returned non-zero         |
| 110_average                 | COMPILE_FAIL | rcc returned non-zero         |
| 111_conversion              | COMPILE_FAIL | rcc returned non-zero         |
| 112_backtrace               | SKIP         | Skipped                       |
| 113_btdll                   | SKIP         | Skipped                       |
| 114_bound_signal            | SKIP         | Skipped                       |
| 115_bound_setjmp            | SKIP         | Skipped                       |
| 116_bound_setjmp2           | SKIP         | Skipped                       |
| 117_builtins                | COMPILE_FAIL | rcc returned non-zero         |
| 118_switch                  | COMPILE_FAIL | rcc returned non-zero         |
| 119_random_stuff            | COMPILE_FAIL | rcc returned non-zero         |
| 120_alias                   | COMPILE_FAIL | rcc returned non-zero         |
| 121_struct_return           | COMPILE_FAIL | rcc returned non-zero         |
| 122_vla_reuse               | COMPILE_FAIL | rcc returned non-zero         |
| 123_vla_bug                 | COMPILE_FAIL | rcc returned non-zero         |
| 124_atomic_counter          | COMPILE_FAIL | rcc returned non-zero         |
| 125_atomic_misc             | MISMATCH     | Output differs                |
| 126_bound_global            | SKIP         | Skipped                       |
| 127_asm_goto                | COMPILE_FAIL | rcc returned non-zero         |
| 128_run_atexit              | COMPILE_FAIL | executable missing            |
| 129_scopes                  | COMPILE_FAIL | rcc returned non-zero         |
| 130_large_argument          | COMPILE_FAIL | rcc returned non-zero         |
| 131_return_struct_in_reg    | COMPILE_FAIL | rcc returned non-zero         |
| 132_bound_test              | COMPILE_FAIL | rcc returned non-zero         |
| 133_old_func                | COMPILE_FAIL | rcc returned non-zero         |
| 134_double_to_signed        | COMPILE_FAIL | rcc returned non-zero         |
| 135_func_arg_struct_compare | COMPILE_FAIL | rcc returned non-zero         |
| 136_atomic_gcc_style        | COMPILE_FAIL | rcc returned non-zero         |
| 137_funcall_struct_args     | COMPILE_FAIL | rcc returned non-zero         |
| 138_arm64_encoding          | SKIP         | Skipped                       |
| 138_narrow_return_promotion | COMPILE_FAIL | rcc returned non-zero         |
| 139_arm64_errors            | SKIP         | Skipped                       |
| 139_narrow_type_conversion  | COMPILE_FAIL | rcc returned non-zero         |
| 140_arm64_extasm            | SKIP         | Skipped                       |
| 140_int_sign_extension      | COMPILE_FAIL | rcc returned non-zero         |
| 141_riscv_asm_pseudo        | SKIP         | Skipped                       |
| 142_riscv_asm_longlong      | SKIP         | Skipped                       |
| 143_riscv_asm_farith        | SKIP         | Skipped                       |
| test_arm64_asm              | SKIP         | Skipped                       |
| test_atomic_op              | COMPILE_FAIL | rcc returned non-zero         |
| test_atomic_op2             | COMPILE_FAIL | rcc returned non-zero         |
| test_bitfields              | COMPILE_FAIL | rcc returned non-zero         |
| test_builtins               | COMPILE_FAIL | rcc returned non-zero         |
| test_elif2                  | COMPILE_FAIL | rcc returned non-zero         |
| test_elif_simple            | COMPILE_FAIL | rcc returned non-zero         |
| test_err                    | PASS         | compile error as expected     |
| test_fallthrough            | COMPILE_FAIL | rcc returned non-zero         |
| test_func                   | COMPILE_FAIL | rcc returned non-zero         |
| test_gperf                  | COMPILE_FAIL | rcc returned non-zero         |
| test_if                     | COMPILE_FAIL | rcc returned non-zero         |
| test_if2                    | COMPILE_FAIL | rcc returned non-zero         |
| test_if3                    | COMPILE_FAIL | rcc returned non-zero         |
| test_if4                    | COMPILE_FAIL | rcc returned non-zero         |
| test_if5                    | COMPILE_FAIL | rcc returned non-zero         |
| test_if6                    | COMPILE_FAIL | rcc returned non-zero         |
| test_if_nested              | COMPILE_FAIL | rcc returned non-zero         |
| test_if_simple              | COMPILE_FAIL | rcc returned non-zero         |
| test_include                | COMPILE_FAIL | rcc returned non-zero         |
| test_include2               | COMPILE_FAIL | rcc returned non-zero         |
| test_loop                   | COMPILE_FAIL | rcc returned non-zero         |
| test_macro                  | COMPILE_FAIL | rcc returned non-zero         |
| test_minimal                | COMPILE_FAIL | rcc returned non-zero         |
| test_nested_if              | COMPILE_FAIL | rcc returned non-zero         |
| test_ptr                    | COMPILE_FAIL | rcc returned non-zero         |
| test_real                   | COMPILE_FAIL | rcc returned non-zero         |
| test_self_include2          | COMPILE_FAIL | rcc returned non-zero         |
| test_signextend             | COMPILE_FAIL | rcc returned non-zero         |
| test_simple                 | COMPILE_FAIL | rcc returned non-zero         |
| test_simple2                | COMPILE_FAIL | rcc returned non-zero         |
| test_str                    | COMPILE_FAIL | rcc returned non-zero         |
| test_struct                 | COMPILE_FAIL | rcc returned non-zero         |
| test_with_comment           | COMPILE_FAIL | rcc returned non-zero         |
