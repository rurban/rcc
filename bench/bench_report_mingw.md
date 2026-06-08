# Windows RCC Benchmark Results

_Generated: 06/09/2026 05:20:16_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         5905 |          647 |       6552 |
| RCC -O1 (optimized)   |         1246 |          664 |       1910 |
| TCC (Tiny C Compiler) |         1231 |          426 |       1657 |
| GCC -O0 (no opt)      |         2220 |          408 |       2628 |
| GCC -O2 (optimized)   |         2236 |          106 |       2342 |
| CLANG -O2 (optimized) |         5031 |          166 |       5197 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 4.8x
- Execute speed : RCC/TCC = 1.52x
- Total : RCC/TCC = 3.95x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
