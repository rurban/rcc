# TCC Test Suite Report for RCC (asm-darwin branch)

## Summary

- **Total runnable**: ~135
- **Passed**: ~131
- **Failed**: 4
- **Pass Rate**: ~97%

## Remaining Failures

| Test | Status | Notes |
|------|--------|-------|
| 73_arm64 | COMPILE_FAIL | 'Invalid register -1' in ARM64 codegen |
| 104_inline | EXEC_FAIL (SIGSEGV) | Inline function codegen |
| 125_atomic_misc | EXEC_FAIL (SIGSEGV) | ARM64 atomics |
| 128_run_atexit | EXEC_FAIL (SIGSEGV) | atexit support |

## Fixed This Session

| Test | Fix | Commit |
|------|-----|--------|
| 22-23_floating_point | Variadic float double-scaling | 95360ea |
| 42_function_pointer | fprintf _ prefix via sym_name() | 95360ea |
| 49_bracket_evaluation | Mach-O BSS vmsize | 3693266 |
| 90_struct-init | .L labels with relocs update UNDEF→SEC_TEXT | 9303dc0 |
| 101_cleanup | x11→x17 allocator conflict + cleanup flow | e7bd7e8 |
| 106_versym | Same-symbol asm_name skip (already present) | - |
| 119_random_stuff | .L labels with ADRP+ADD relocs | 9303dc0 |
| 140_arm64_extasm | Inline asm semicolons (already present) | - |

