# Windows RCC Benchmark Results

_Generated: 05/03/2026 04:58:26_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         6325 |          563 |       6888 |
| RCC -O1 (optimized)   |         1280 |          554 |       1834 |
| TCC (Tiny C Compiler) |         1250 |          428 |       1678 |
| GCC -O0 (no opt)      |         2256 |          417 |       2673 |
| GCC -O2 (optimized)   |         2247 |          116 |       2363 |
| CLANG -O2 (optimized) |         4069 |          170 |       4239 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 5.06x
- Execute speed : RCC/TCC = 1.32x
- Total : RCC/TCC = 4.1x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
