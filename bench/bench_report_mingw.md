# Windows RCC Benchmark Results

_Generated: 06/09/2026 10:49:10_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2744 |          605 |       3349 |
| RCC -O1 (optimized)   |         1253 |          607 |       1860 |
| TCC (Tiny C Compiler) |         1255 |          430 |       1685 |
| GCC -O0 (no opt)      |         1246 |          419 |       1665 |
| GCC -O2 (optimized)   |         1240 |          116 |       1356 |
| CLANG -O2 (optimized) |         1265 |          170 |       1435 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.19x
- Execute speed : RCC/TCC = 1.41x
- Total : RCC/TCC = 1.99x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
