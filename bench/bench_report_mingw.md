# Windows RCC Benchmark Results

_Generated: 05/02/2026 20:27:02_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2759 |          649 |       3408 |
| RCC -O1 (optimized)   |         1215 |          649 |       1864 |
| TCC (Tiny C Compiler) |         1217 |          670 |       1887 |
| GCC -O0 (no opt)      |         1217 |          556 |       1773 |
| GCC -O2 (optimized)   |         1231 |          135 |       1366 |
| CLANG -O2 (optimized) |         1235 |          307 |       1542 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.27x
- Execute speed : RCC/TCC = 0.97x
- Total : RCC/TCC = 1.81x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
