# Windows RCC Benchmark Results

_Generated: 05/07/2026 15:08:18_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2920 |          549 |       3469 |
| RCC -O1 (optimized)   |         1258 |          544 |       1802 |
| TCC (Tiny C Compiler) |         1270 |          428 |       1698 |
| GCC -O0 (no opt)      |         1248 |          420 |       1668 |
| GCC -O2 (optimized)   |         1251 |          117 |       1368 |
| CLANG -O2 (optimized) |         1265 |          169 |       1434 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.3x
- Execute speed : RCC/TCC = 1.28x
- Total : RCC/TCC = 2.04x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
