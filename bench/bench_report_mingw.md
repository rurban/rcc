# Windows RCC Benchmark Results

_Generated: 05/30/2026 14:35:33_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2666 |          600 |       3266 |
| RCC -O1 (optimized)   |         1284 |          617 |       1901 |
| TCC (Tiny C Compiler) |         1306 |          432 |       1738 |
| GCC -O0 (no opt)      |         1299 |          428 |       1727 |
| GCC -O2 (optimized)   |         1281 |          117 |       1398 |
| CLANG -O2 (optimized) |         1300 |          171 |       1471 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.04x
- Execute speed : RCC/TCC = 1.39x
- Total : RCC/TCC = 1.88x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
