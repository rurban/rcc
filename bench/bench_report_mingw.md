# Windows RCC Benchmark Results

_Generated: 06/02/2026 06:58:23_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2666 |          751 |       3417 |
| RCC -O1 (optimized)   |         1222 |          750 |       1972 |
| TCC (Tiny C Compiler) |         1223 |          673 |       1896 |
| GCC -O0 (no opt)      |         1226 |          556 |       1782 |
| GCC -O2 (optimized)   |         1249 |          134 |       1383 |
| CLANG -O2 (optimized) |         1226 |          307 |       1533 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.18x
- Execute speed : RCC/TCC = 1.12x
- Total : RCC/TCC = 1.8x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
