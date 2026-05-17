# Windows RCC Benchmark Results

_Generated: 05/17/2026 07:30:31_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         6734 |          746 |       7480 |
| RCC -O1 (optimized)   |         1266 |          747 |       2013 |
| TCC (Tiny C Compiler) |         1270 |          507 |       1777 |
| GCC -O0 (no opt)      |         2269 |          475 |       2744 |
| GCC -O2 (optimized)   |         2295 |          128 |       2423 |
| CLANG -O2 (optimized) |         3275 |          207 |       3482 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 5.3x
- Execute speed : RCC/TCC = 1.47x
- Total : RCC/TCC = 4.21x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
