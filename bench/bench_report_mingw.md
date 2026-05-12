# Windows RCC Benchmark Results

_Generated: 05/12/2026 07:58:04_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3088 |          617 |       3705 |
| RCC -O1 (optimized)   |         1254 |          605 |       1859 |
| TCC (Tiny C Compiler) |         1269 |          431 |       1700 |
| GCC -O0 (no opt)      |         1256 |          415 |       1671 |
| GCC -O2 (optimized)   |         1256 |          117 |       1373 |
| CLANG -O2 (optimized) |         3305 |          172 |       3477 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.43x
- Execute speed : RCC/TCC = 1.43x
- Total : RCC/TCC = 2.18x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
