# Windows RCC Benchmark Results

_Generated: 06/05/2026 12:10:24_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         4743 |          614 |       5357 |
| RCC -O1 (optimized)   |         1260 |          610 |       1870 |
| TCC (Tiny C Compiler) |         1260 |          429 |       1689 |
| GCC -O0 (no opt)      |         2281 |          418 |       2699 |
| GCC -O2 (optimized)   |         2302 |          117 |       2419 |
| CLANG -O2 (optimized) |         1273 |          170 |       1443 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 3.76x
- Execute speed : RCC/TCC = 1.43x
- Total : RCC/TCC = 3.17x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
