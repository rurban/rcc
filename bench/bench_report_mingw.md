# Windows RCC Benchmark Results

_Generated: 05/14/2026 10:50:24_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         9098 |          739 |       9837 |
| RCC -O1 (optimized)   |         1258 |          743 |       2001 |
| TCC (Tiny C Compiler) |         1247 |          498 |       1745 |
| GCC -O0 (no opt)      |         2264 |          464 |       2728 |
| GCC -O2 (optimized)   |         2265 |          118 |       2383 |
| CLANG -O2 (optimized) |         3863 |          188 |       4051 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 7.3x
- Execute speed : RCC/TCC = 1.48x
- Total : RCC/TCC = 5.64x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
