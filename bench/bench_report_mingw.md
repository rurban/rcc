# Windows RCC Benchmark Results

_Generated: 05/31/2026 04:12:24_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2755 |          758 |       3513 |
| RCC -O1 (optimized)   |         1228 |          751 |       1979 |
| TCC (Tiny C Compiler) |         1219 |          671 |       1890 |
| GCC -O0 (no opt)      |         1219 |          555 |       1774 |
| GCC -O2 (optimized)   |         1258 |          134 |       1392 |
| CLANG -O2 (optimized) |         1224 |          309 |       1533 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.26x
- Execute speed : RCC/TCC = 1.13x
- Total : RCC/TCC = 1.86x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
